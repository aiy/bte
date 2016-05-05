/*
http://en.wikipedia.org/wiki/Behavior_Trees_(Artificial_Intelligence,_Robotics_and_Control)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // for isspace
#include <unistd.h> // for dup

#include <libxml/debugXML.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "uthash.h"

#define ULLOG_DEST (ULLOG_DEST_STDOUT)
//#define ULLOG_DEST (ULLOG_DEST_STDOUT | ULLOG_DEST_STDERR | ULLOG_DEST_SYSLOG)
#define ULLOG_LEVEL ULLOG_NOTICE
//#define ULLOG_LEVEL ULLOG_DEBUG
#include "ullog.h"
static g_debug = 0;

enum {
  ACTION,
} node_type;

enum os_type {
  UNIX,
};
typedef enum os_type os_t;

// return code 
enum rc_type {
  RC_SUCCESS, // clean success
  RC_FAILURE, // clean failure
  RC_RUNNING, // running
  RC_ERROR, // unexpected failure
  RC_UNKNOWN, // unknown code
};
typedef enum rc_type rc_t;

struct _return_code {
  const int code;
  const char *str;
};
typedef struct _return_code rcs_t;

const rcs_t rcs_str_mapping[] = {
    {RC_SUCCESS, "RC_SUCCESS"},
    {RC_FAILURE, "RC_FAILURE"},
    {RC_RUNNING, "RC_RUNNING"},
    {RC_ERROR, "RC_ERROR"},
    {RC_UNKNOWN, NULL},
};

struct action_node {
  char *value;
  int type;
  int os;
};

struct _fp_table {
  char id[255];
  FILE *fp;
  UT_hash_handle hh; /* makes this structure hashable */
};
typedef struct _fp_table fp_table_t;

static fp_table_t *fp_table = NULL;

static rc_t processFile(const char *filename);
static rc_t processRootNode(xmlNodePtr node);
static rc_t processNode(xmlNodePtr node);
static rc_t processSequenceNode(xmlNodePtr node);
static rc_t processSelectNode(xmlNodePtr node);
static rc_t processActionNode(xmlNodePtr node);

// system specific
static rc_t oldexecActionCmd(xmlChar *value);
static rc_t processActionExec(xmlNodePtr node);

static const char * rc2rstr(const int rc) {
  int i = 0;
  for (; rcs_str_mapping[i].code != RC_UNKNOWN; ++i) {
    if(rcs_str_mapping[i].code == rc) {
      return(rcs_str_mapping[i].str);
    }
  }
  return(NULL);
}

static char * _gen_node_id(void) {
  char *id_str = NULL;
  if((id_str = malloc(255)) == NULL) {
    return(NULL);
  }
  srand(time(NULL));
  if (sprintf(id_str, "%d", rand()) == -1) {
    return(NULL);
  }

  return(id_str);
}

static char * _trimwhitespace(char *str) {
  char *end;

  // Trim leading space
  while (isspace(*str))
    str++;

  if (*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while (end > str && isspace(*end))
    end--;

  // Write new null terminator
  *(end + 1) = 0;

  return str;
}

static void _xmlDump(xmlNode *node) {
  ullog_debug("enter");

  if (g_debug) {
    /*
    xmlChar *name, *value;

    name = xmlTextReaderName(reader);
    if (name == NULL)
      name = xmlStrdup(BAD_CAST "--");
    value = xmlTextReaderValue(reader);

    log_debug("%s: element address %p depth %d type %d name %s empty %d",
        __FUNCTION__, xmlTextReaderCurrentNode(reader),
        xmlTextReaderDepth(reader), xmlTextReaderNodeType(reader), name,
        xmlTextReaderIsEmptyElement(reader));
    if (value) {
      if (strlen((const char *) value) > 0) {
        log_debug(" value '%s'", value);
      }
    }
    log_debug("\n");
    */

    if (node) {
      xmlDebugDumpNode(stdout, node, -1);
    }
    /*
    if (name)
      xmlFree(name);
    if (value)
      xmlFree(value);
      */
  }

  ullog_debug("exit");
}

static rc_t oldexecActionCmd(xmlChar *value) {
	ullog_debug("enter");

  rc_t task_rc = RC_FAILURE;
  int rc = 1;

  if (value) {
    _trimwhitespace((char *) value);
    ullog_debug("executing action '%s'", value);
    // TODO read stdout
    dup2(1, 2);  //redirects stderr to stdout below this line.
    rc = system((const char *) value);
    if (rc == 0) {
      task_rc = RC_SUCCESS;
    }
  } else {
		ullog_err("cannot read command value or it is empty");
		task_rc = RC_ERROR;
		goto bail;
	}

bail:
  ullog_debug("task_rc %d", task_rc);

	ullog_debug("exit");
  return task_rc;
}

static rc_t processActionExec(xmlNodePtr node) {
	ullog_debug("enter");

  rc_t task_rc = RC_FAILURE;
  int rc = 1;
  xmlChar *node_id = NULL;
  xmlChar *action_value = NULL;
  char exec_path[255] = "";
	xmlChar *action_state = NULL;
	char exec_out_buff[255] = "";
  fp_table_t *fp_table_item = NULL;
  fp_table_t *fp_table_item_tmp = NULL;
  int c = 0;

  node_id = xmlGetProp(node, "id");
	if (node_id && (strlen(node_id) > 0)) {
		ullog_debug("node id '%s'", node_id);
	} else {
		ullog_debug("generating node id");
		node_id = _gen_node_id();
		if(!node_id) {
			ullog_err("cannot generate node id");
			task_rc = RC_ERROR;
			goto bail;
		}
		ullog_debug("node id '%s'", node_id);
		if(!xmlNewProp(node, "id", node_id)) {
			ullog_err("cannot write node id to tree");
			task_rc = RC_ERROR;
			goto bail;
		}
	}

  action_state = xmlGetProp(node, "_state_");
	if (action_state && (strlen(action_state) > 0)) {
		ullog_debug("action state '%s'", action_state);
		ullog_debug("action is running");
	} else {
		ullog_debug("action is not running");
		action_value = xmlNodeGetContent(node);
  	if (action_value) {
    	_trimwhitespace((char *) action_value);
			if (strlen(action_value) == 0) {
				ullog_err("cannot read command value or it is empty");
				task_rc = RC_ERROR;
				goto bail;
			}

   		ullog_debug("create store fp item");
      fp_table_item = (fp_table_t*)malloc(sizeof(fp_table_t));
      if(!fp_table_item) {
				ullog_err("cannot create fp table item");
				task_rc = RC_ERROR;
				goto bail;
      }

   		ullog_debug("action value '%s'", action_value);
      strncat(exec_path, action_value, sizeof(exec_path)); 
      strncat(exec_path, " 2>&1", sizeof(exec_path)); 
   		ullog_debug("executing action '%s'", exec_path);
    	if(!(fp_table_item->fp = popen(exec_path, "r"))) {
				ullog_err("cannot execute command '%s'", action_value);
				task_rc = RC_ERROR;
				goto bail;
			}

   		ullog_debug("start store fp in fp table");
      strncpy(fp_table_item->id, node_id, 255);
   		ullog_debug("id '%s'", fp_table_item->id);
   		ullog_debug("fp '%p'", fp_table_item->fp);
      HASH_ADD_STR(fp_table, id, fp_table_item);
   		ullog_debug("done store fp in fp table");

  	} else {
			ullog_err("cannot read command value or it is empty");
			task_rc = RC_ERROR;
			goto bail;
		}
	}

#if 0
  // list fps in table for debugging only
  HASH_ITER(hh, fp_table, fp_table_item, fp_table_item_tmp) {
   	ullog_debug("id '%s'", fp_table_item->id);
   	ullog_debug("fp '%p'", fp_table_item->fp);
  }
#endif

  if(!fp_table_item) {
    HASH_FIND_STR(fp_table, node_id, fp_table_item);
  }
  if(!fp_table_item) {
		ullog_err("cannot find open fp for node id '%s'", node_id);
		task_rc = RC_ERROR;
		goto bail;
  }

	ullog_debug("start reading exec output");
	if(fgets(exec_out_buff, 255, fp_table_item->fp) == NULL) {
		if(!feof(fp_table_item->fp) || ferror(fp_table_item->fp)) {
			ullog_err("cannot read output of command '%s'", action_value);
			task_rc = RC_ERROR;
			goto bail;
		}
	}
	ullog_debug("done reading exec output");

  // check eof
  c = getc(fp_table_item->fp); 
  ungetc(c, fp_table_item->fp); 
	if(feof(fp_table_item->fp)) {
   	ullog_debug("exec action output eof");
		if(pclose(fp_table_item->fp) == 0) {
			task_rc = RC_SUCCESS;
    } else {
			task_rc = RC_FAILURE;
    }
    HASH_DEL(fp_table, fp_table_item);
    free(fp_table_item);
		if(!xmlNewProp(node, "_state_", "")) {
			ullog_err("cannot write node state to tree");
			task_rc = RC_ERROR;
			goto bail;
		}
	} else {
   	ullog_debug("no exec action output eof");
		task_rc = RC_RUNNING;
		if(!xmlNewProp(node, "_state_", "running")) {
			ullog_err("cannot write node state to tree");
			task_rc = RC_ERROR;
			goto bail;
		}
  }
 	ullog_debug("current exec output '%s'", exec_out_buff);
	printf("%s", exec_out_buff);


bail:
  ullog_debug("task_rc %s", rc2rstr(task_rc));
  if (node_id) xmlFree(node_id);
  if (action_value) xmlFree(action_value);
  if (action_state) xmlFree(action_state);
  if(fp_table && fp_table_item) {
    if(task_rc == RC_ERROR) {
			pclose(fp_table_item->fp);
      HASH_DEL(fp_table, fp_table_item);
      free(fp_table_item);
    }
  }

	ullog_debug("exit");
  return(task_rc);
}

static rc_t processActionNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_FAILURE;
  xmlNodePtr cur_node = NULL;

  _xmlDump(node);

  for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
    _xmlDump(cur_node);
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp(cur_node->name, (const xmlChar *) "exec") == 0) {
        ullog_debug("action node address '%p'", cur_node);
        task_rc = processActionExec(cur_node);
        break;
      } else {
        ullog_err("node '%s' is not supported", cur_node->name);
        _xmlDump(cur_node);
        task_rc = RC_ERROR;
        goto bail;
      }
    }
  }

bail:

  ullog_debug("task_rc %s", rc2rstr(task_rc));

  ullog_debug("exit");
  return(task_rc);
}

static rc_t processSequenceNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_SUCCESS;
  xmlNodePtr cur_node = NULL;

  _xmlDump(node);

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    _xmlDump(cur_node);
    if (cur_node->type == XML_ELEMENT_NODE) {
      task_rc = processNode(cur_node);
      if (task_rc == RC_FAILURE || task_rc == RC_ERROR) {
        goto bail;
      }
    }
  }

bail:

  ullog_debug("task_rc %s", rc2rstr(task_rc));

  ullog_debug("exit");
  return(task_rc);
}

static rc_t processSelectNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_SUCCESS;
  xmlNodePtr cur_node = NULL;

  _xmlDump(node);

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    _xmlDump(cur_node);
    if (cur_node->type == XML_ELEMENT_NODE) {
      task_rc = processNode(cur_node);
      if (task_rc == RC_SUCCESS || task_rc == RC_ERROR) {
        goto bail;
      }
    }
  }

bail:

  ullog_debug("task_rc %s", rc2rstr(task_rc));

  ullog_debug("exit");
  return(task_rc);
}

static rc_t processNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_SUCCESS;

  _xmlDump(node);

  if (xmlStrcmp(node->name, (const xmlChar *) "action") == 0) {
    ullog_debug("action node address '%p'", node);
    task_rc = processActionNode(node);
  } else if (xmlStrcmp(node->name, (const xmlChar *) "sequence") == 0) {
    ullog_debug("sequence node address '%p'", node);
    task_rc = processSequenceNode(node->children);
  } else if (xmlStrcmp(node->name, (const xmlChar *) "select") == 0) {
    ullog_debug("select node address '%p'", node);
    task_rc = processSelectNode(node->children);
  } else {
    ullog_err("node '%s' is not supported", node->name);
    _xmlDump(node);
    task_rc = RC_ERROR;
    goto bail;
  }

bail:

  ullog_debug("task_rc %s", rc2rstr(task_rc));

  ullog_debug("exit");
  return(task_rc);
}

static rc_t processRootNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_SUCCESS;
  xmlNodePtr cur_node = NULL;

  //_xmlDump(node);

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    _xmlDump(cur_node);
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp(cur_node->name, (const xmlChar *) "bt") == 0) {
        ullog_debug("node is bt");
        task_rc = processSequenceNode(cur_node->children);
        // only one bt node
        goto bail;
      } else {
        ullog_err("node '%s' is not supported", cur_node->name);
        _xmlDump(cur_node);
        task_rc = RC_ERROR;
        goto bail;
      }
    }
  }

bail:

  ullog_debug("task_rc %s", rc2rstr(task_rc));

  ullog_debug("exit");
  return(task_rc);
}

rc_t processFile(const char *filename) {
  ullog_debug("enter");

  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL;
  rc_t task_rc = RC_FAILURE;
	int run_i = 1;

  ullog_debug("start xmlReadFile");
  doc = xmlReadFile(filename, NULL, 0);
  if (doc == NULL) {
    ullog_err("unable to open file %s", filename);
    task_rc = RC_ERROR;
    goto bail;
  }
  ullog_debug("done xmlReadFile: doc %p", doc);

  rootNode = xmlDocGetRootElement(doc);
  if (rootNode == NULL) {
    ullog_err("unable to open file %s: no root element", filename);
    task_rc = RC_ERROR;
    goto bail;
  }
  // process root as sequence
  // need to keep running if it is still in RUNNING state
  ullog_debug("start processRootNode");
	do {
		ullog_debug("start run iteration %d", run_i);
		task_rc = processRootNode(rootNode);
		ullog_debug("done run iteration %d task_rc %s", run_i, rc2rstr(task_rc));
		++run_i;
	} while (task_rc == RC_RUNNING);
  ullog_debug("done processRootNode rc %s", rc2rstr(task_rc));

bail:
  if (doc) xmlFreeDoc(doc);
  xmlCleanupParser();
  ullog_debug("task_rc %s", rc2rstr(task_rc));

  ullog_debug("exit");
  return(task_rc);
}

int main(int argc, char *argv[]) {
	ullog_init("bte");
  ullog_debug("enter bte");

  rc_t task_rc = RC_FAILURE;

  if (argc < 2) {
    ullog_err("provide file");
    task_rc = RC_ERROR;
    goto bail;
  }
  if (argc == 3) {
    if (strncmp(argv[2], "-d", 2) == 0) {
      ullog_debug("enable debug");
      g_debug = 1;
    }
  }
  ullog_debug("done process cli");

  ullog_debug("start processFile");
  task_rc = processFile(argv[1]);
  ullog_debug("done processFile rc %s", rc2rstr(task_rc));

bail:

  ullog_debug("rc %s", rc2rstr(task_rc));
  ullog_debug("exit bte");

	ullog_deinit();
  return(task_rc);
}

// EOF
