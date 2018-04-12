/*
http://en.wikipedia.org/wiki/Behavior_Trees_(Artificial_Intelligence,_Robotics_and_Control)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> // for isspace
#include <unistd.h> // for dup
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/debugXML.h>

#include <expect.h>

#include "uthash.h"

#define ULLOG_DEST (ULLOG_DEST_STDOUT)
//#define ULLOG_DEST (ULLOG_DEST_STDOUT | ULLOG_DEST_STDERR)
//#define ULLOG_DEST (ULLOG_DEST_STDOUT | ULLOG_DEST_STDERR | ULLOG_DEST_SYSLOG)
#define ULLOG_LEVEL ULLOG_NOTICE
//#define ULLOG_LEVEL ULLOG_DEBUG
#include "ullog.h"
static int g_debug = 0;

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
  const char *id;
  FILE *fp;
  int fd;
  UT_hash_handle hh; /* makes this structure hashable */
};
typedef struct _fp_table fp_table_t;

static fp_table_t *fp_table = NULL;

static rc_t processFile(const char *filename);
static rc_t processRootNode(xmlNodePtr node);
static rc_t processNode(xmlNodePtr node);
static rc_t processDecoratorNode(xmlNodePtr node);
static rc_t processDecoratorSucceederNode(xmlNodePtr node);
static rc_t processSequenceNode(xmlNodePtr node);
static rc_t processSelectNode(xmlNodePtr node);
static rc_t processActionLeaf(xmlNodePtr node);

// system specific
static rc_t processActionExec(xmlNodePtr node);
static rc_t processActionOpen(xmlNodePtr node);
static rc_t processActionClose(xmlNodePtr node);

static const char * rc2rstr(const int rc) {
  int i = 0;
  for (; rcs_str_mapping[i].code != RC_UNKNOWN; ++i) {
    if(rcs_str_mapping[i].code == rc) {
      return(rcs_str_mapping[i].str);
    }
  }
  return(NULL);
}

static int rand_init = 0;
static char * _gen_node_id(void) {
  char *id_str = NULL; 
  if((id_str = malloc(255)) == NULL) { 
    return(NULL); 
  }
  if(!rand_init) {
    rand_init = 1;
    srand(time(NULL));
  }
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

static void _xmlDump(xmlNode *node, int recursive) {
  if (node) {
    if(recursive) {
      xmlDebugDumpNode(stdout, node, -1);
    } else {
      xmlDebugDumpOneNode(stdout, node, -1);
    }
  }

  if (g_debug) {
    ullog_debug("enter");
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

    /*
    if (name)
      xmlFree(name);
    if (value)
      xmlFree(value);
      */
    ullog_debug("exit");
  }
}

static rc_t processActionExec(xmlNodePtr node) {
    ullog_debug("enter");

    rc_t task_rc = RC_FAILURE;
    xmlChar *node_id = NULL;
    xmlChar *action_value = NULL;
    char exec_path[PATH_MAX] = "";
	xmlChar *action_state = NULL;
	char exec_out_buff[255] = "";
    fp_table_t *fp_table_item = NULL;
    // enable to print hash
    // fp_table_t *fp_table_item_tmp = NULL;
    int c = 0;

    node_id = xmlGetProp(node, (const xmlChar *) "id");
	if (node_id && (strlen((const char *) node_id) > 0)) {
		ullog_debug("node id '%s'", node_id);
	} else {
		ullog_debug("generating node id");
	    node_id = (xmlChar *) _gen_node_id();
		if(!node_id) {
			ullog_err("cannot generate node id");
			task_rc = RC_ERROR;
			goto bail;
		}
		if(!xmlNewProp(node,  (const xmlChar *) "id", node_id)) {
			ullog_err("cannot write node id to tree");
			task_rc = RC_ERROR;
			goto bail;
		}
		ullog_debug("new generated node id '%s'", node_id);
	}

    action_state = xmlGetProp(node,  (const xmlChar *) "_state_");
	ullog_debug("action state '%s'", action_state);
	if (action_state && 
        (strncmp((const char *) action_state, "running", strlen("running")) == 0)) {
		ullog_debug("action is running");
	} else if (action_state && 
        (strncmp((const char *) action_state, "success", strlen("success")) == 0)) {
		ullog_debug("action is success");
	    task_rc = RC_SUCCESS;
		goto bail;
	} else {
		ullog_debug("action is not set");
		action_value = xmlNodeGetContent(node);
  	    if (action_value) {
            _trimwhitespace((char *) action_value);
            if (strlen((const char *) action_value) == 0) {
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
            strncat(exec_path, (const char *) action_value, PATH_MAX);
            strncat(exec_path, " 2>&1", PATH_MAX);
            ullog_debug("executing action '%s'", exec_path);
            if(!(fp_table_item->fp = popen(exec_path, "r"))) {
                ullog_err("cannot execute command '%s'", action_value);
                task_rc = RC_ERROR;
                goto bail;
            }

            ullog_debug("start store fp in fp table");
            fp_table_item->id = (const char *) node_id;
            ullog_debug("id '%s'", fp_table_item->id);
            ullog_debug("fp '%p'", fp_table_item->fp);
            HASH_ADD_KEYPTR(hh, fp_table, 
            fp_table_item->id, strlen(fp_table_item->id), fp_table_item);
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
        ullog_debug("search fp_table_item");
        HASH_FIND_STR(fp_table, (const char *) node_id, fp_table_item);
    }
    ullog_debug("node_id '%p'", node_id);
    ullog_debug("fp_table '%p'", fp_table);
    ullog_debug("fp_table_item '%p'", fp_table_item);
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
        ullog_debug("closing fp '%p'", fp_table_item->fp);
            if(pclose(fp_table_item->fp) == 0) {
                task_rc = RC_SUCCESS;
        } else {
                task_rc = RC_FAILURE;
        }
        HASH_DEL(fp_table, fp_table_item);
        free(fp_table_item);
        if(!xmlSetProp(node, (const xmlChar *) "_state_", (const xmlChar *) "success")) {
            ullog_err("cannot write node state to tree");
            task_rc = RC_ERROR;
            goto bail;
        }
	} else {
        ullog_debug("no exec action output eof");
        task_rc = RC_RUNNING;
        if (action_state) {
          if(!xmlSetProp(node, (const xmlChar *) "_state_", (const xmlChar *) "running")) {
            ullog_err("cannot update node state to value 'running'");
            task_rc = RC_ERROR;
            goto bail;
          }
        } else {
          if(!xmlNewProp(node, (const xmlChar *) "_state_",(const xmlChar *)  "running")) {
            ullog_err("cannot write node state value 'running'");
            task_rc = RC_ERROR;
            goto bail;
          }
        }
    }
 	ullog_debug("current exec output '%s'", exec_out_buff);
	printf("%s", exec_out_buff);

    bail:
    ullog_debug("task_rc %s", rc2rstr(task_rc));
    //if (node_id) xmlFree(node_id);
    if (action_value) xmlFree(action_value);
    if (action_state) xmlFree(action_state);
    if(fp_table && fp_table_item) {
        if(task_rc == RC_ERROR) {
            if(fp_table_item->fp) pclose(fp_table_item->fp);
            HASH_DEL(fp_table, fp_table_item);
            free(fp_table_item);
        }
    }

	ullog_debug("exit");
    return(task_rc);
}

static rc_t processActionOpen(xmlNodePtr node) {
    ullog_debug("enter");

    rc_t task_rc = RC_FAILURE;
    xmlChar *node_id = NULL;
    xmlChar *stream_id = NULL;
    xmlChar *action_value = NULL;
    //char exec_path[PATH_MAX] = "";
	xmlChar *action_state = NULL;
	//char exec_out_buff[255] = "";
    fp_table_t *fp_table_item = NULL;
    // enable to print hash
    // fp_table_t *fp_table_item_tmp = NULL;
    //int c = 0;
    int opt = 0;
    char *argvcp = NULL;
	// FIXME fix array
    char argv[255][255] = {};
    char *token = NULL;
    const char delim[] = " ";
	int i = 0;

    node_id = xmlGetProp(node, (const xmlChar *) "id");
	if (node_id && (strlen((const char *) node_id) > 0)) {
		ullog_debug("node id '%s'", node_id);
	} else {
		ullog_debug("generating node id");
	    node_id = (xmlChar *) _gen_node_id();
		if(!node_id) {
			ullog_err("cannot generate node id");
			task_rc = RC_ERROR;
			goto bail;
		}
		if(!xmlNewProp(node,  (const xmlChar *) "id", node_id)) {
			ullog_err("cannot write node id to tree");
			task_rc = RC_ERROR;
			goto bail;
		}
		ullog_debug("new generated node id '%s'", node_id);
	}
    stream_id = xmlGetProp(node, (const xmlChar *) "stream_id");
	if (stream_id && (strlen((const char *) stream_id) > 0)) {
		ullog_debug("stream id '%s'", stream_id);
	} else {
        ullog_err("cannot read node stream id");
        task_rc = RC_ERROR;
        goto bail;
	}

    action_state = xmlGetProp(node,  (const xmlChar *) "_state_");
	ullog_debug("action state '%s'", action_state);
	if (action_state && 
        (strncmp((const char *) action_state, "running", strlen("running")) == 0)) {
		ullog_debug("action is running");
	} else if (action_state && 
        (strncmp((const char *) action_state, "success", strlen("success")) == 0)) {
		ullog_debug("action is success");
	    task_rc = RC_SUCCESS;
		goto bail;
	} else {
		ullog_debug("action is not set");
		action_value = xmlNodeGetContent(node);
  	    if (action_value) {
            _trimwhitespace((char *) action_value);
            if (strlen((const char *) action_value) == 0) {
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
            argvcp = strdup((const char *) action_value);
			token = strtok(argvcp, delim);
			while (token != NULL) {
				snprintf(argv[i], 255, "%s", token);
				token = strtok(NULL, delim);
				++i;
			}

            if(!(fp_table_item->fd = exp_spawnl(argv[0], argv))) {
                ullog_err("cannot execute command '%s'", action_value);
				task_rc = RC_FAILURE;
                goto bail;
            }
			opt = fcntl(fp_table_item->fd, F_GETFL);
			if (opt < 0) {
                ullog_err("cannot F_GETFL on open command '%s'", action_value);
                task_rc = RC_ERROR;
                goto bail;
			}
			opt |= O_NONBLOCK;
			if (fcntl(fp_table_item->fd, F_SETFL, opt) < 0) {
                ullog_err("cannot F_SETFL on open command '%s'", action_value);
                task_rc = RC_ERROR;
                goto bail;
			}
			task_rc = RC_SUCCESS;

            ullog_debug("start store fp in fp table");
            fp_table_item->id = (const char *) stream_id;
            ullog_debug("id '%s'", fp_table_item->id);
            ullog_debug("fp '%p'", fp_table_item->fp);
            ullog_debug("fd '%d'", fp_table_item->fd);
            HASH_ADD_KEYPTR(hh, fp_table, 
            fp_table_item->id, strlen(fp_table_item->id), fp_table_item);
            ullog_debug("done store fp in fp table");

            if(!xmlSetProp(node, (const xmlChar *) "_state_", (const xmlChar *) "success")) {
                ullog_err("cannot write node state to tree");
                task_rc = RC_ERROR;
                goto bail;
            }

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
        ullog_debug("search fp_table_item");
        HASH_FIND_STR(fp_table, (const char *) stream_id, fp_table_item);
    }
    ullog_debug("stream_id '%p'", stream_id);
    ullog_debug("fp_table '%p'", fp_table);
    ullog_debug("fp_table_item '%p'", fp_table_item);
    if(!fp_table_item) {
        ullog_err("cannot find open stream id for node id '%s'", node_id);
        task_rc = RC_ERROR;
        goto bail;
    }

    bail:
    ullog_debug("task_rc %s", rc2rstr(task_rc));
    //if (node_id) xmlFree(node_id);
    if (action_value) xmlFree(action_value);
    if (action_state) xmlFree(action_state);
    if(fp_table && fp_table_item) {
        if(task_rc == RC_ERROR) {
            if(fp_table_item->fd) close(fp_table_item->fd);
            HASH_DEL(fp_table, fp_table_item);
            free(fp_table_item);
        }
    }

	ullog_debug("exit");
    return(task_rc);
}

static rc_t processActionClose(xmlNodePtr node) {
    ullog_debug("enter");

    rc_t task_rc = RC_FAILURE;
    xmlChar *node_id = NULL;
    xmlChar *stream_id = NULL;
    xmlChar *action_value = NULL;
    //char exec_path[PATH_MAX] = "";
	xmlChar *action_state = NULL;
	//char exec_out_buff[255] = "";
    fp_table_t *fp_table_item = NULL;
    // enable to print hash
    // fp_table_t *fp_table_item_tmp = NULL;

    node_id = xmlGetProp(node, (const xmlChar *) "id");
	if (node_id && (strlen((const char *) node_id) > 0)) {
		ullog_debug("node id '%s'", node_id);
	} else {
		ullog_debug("generating node id");
	    node_id = (xmlChar *) _gen_node_id();
		if(!node_id) {
			ullog_err("cannot generate node id");
			task_rc = RC_ERROR;
			goto bail;
		}
		if(!xmlNewProp(node,  (const xmlChar *) "id", node_id)) {
			ullog_err("cannot write node id to tree");
			task_rc = RC_ERROR;
			goto bail;
		}
		ullog_debug("new generated node id '%s'", node_id);
	}
    stream_id = xmlGetProp(node, (const xmlChar *) "stream_id");
    if (stream_id && (strlen((const char *) stream_id) > 0)) {
        ullog_debug("stream id '%s'", stream_id);
    } else {
        ullog_err("cannot read node stream id");
        task_rc = RC_ERROR;
        goto bail;
    }

    action_state = xmlGetProp(node,  (const xmlChar *) "_state_");
	ullog_debug("action state '%s'", action_state);
	if (action_state && 
        (strncmp((const char *) action_state, "running", strlen("running")) == 0)) {
		ullog_debug("action is running");
	} else if (action_state && 
        (strncmp((const char *) action_state, "success", strlen("success")) == 0)) {
		ullog_debug("action is success");
	    task_rc = RC_SUCCESS;
		goto bail;
	} else {
		ullog_debug("action is not set");
		action_value = xmlNodeGetContent(node);
  	    if (action_value) {
// no values in close action            
#if 0
            _trimwhitespace((char *) action_value);
            if (strlen((const char *) action_value) == 0) {
                ullog_err("cannot read command value or it is empty");
                task_rc = RC_ERROR;
                goto bail;
            }
            ullog_debug("action value '%s'", action_value);
            strncat(exec_path, (const char *) action_value, PATH_MAX);
#endif

			if(!fp_table_item) {
				ullog_debug("search fp_table_item");
				HASH_FIND_STR(fp_table, (const char *) stream_id, fp_table_item);
			}
			ullog_debug("stream_id '%p'", stream_id);
			ullog_debug("fp_table '%p'", fp_table);
			ullog_debug("fp_table_item '%p'", fp_table_item);
			if(!fp_table_item) {
				ullog_err("cannot find open stream id for node id '%s'", node_id);
				task_rc = RC_ERROR;
				goto bail;
			}

			task_rc = RC_SUCCESS;
            if(fp_table_item->fd) {
                errno = 0;
                if(close(fp_table_item->fd)) {
					task_rc = RC_FAILURE;
				}
            }

			HASH_DEL(fp_table, fp_table_item);
			free(fp_table_item);
			if(!xmlSetProp(node, (const xmlChar *) "_state_", (const xmlChar *) "success")) {
				ullog_err("cannot write node state to tree");
				task_rc = RC_ERROR;
				goto bail;
			}


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


    bail:
    ullog_debug("task_rc %s", rc2rstr(task_rc));
    //if (node_id) xmlFree(node_id);
    if (action_value) xmlFree(action_value);
    if (action_state) xmlFree(action_state);
    if(fp_table && fp_table_item) {
        if(task_rc == RC_ERROR) {
            if(fp_table_item->fd) close(fp_table_item->fd);
            HASH_DEL(fp_table, fp_table_item);
            free(fp_table_item);
        }
    }

	ullog_debug("exit");
    return(task_rc);
}

static rc_t processActionLeaf(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_FAILURE;
  xmlNodePtr cur_node = NULL;

  for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp(cur_node->name, (const xmlChar *) "exec") == 0) {
        ullog_debug("action node address '%p'", cur_node);
        task_rc = processActionExec(cur_node);
        break;
      } else if (xmlStrcmp(cur_node->name, (const xmlChar *) "open") == 0) {
        ullog_debug("action node address '%p'", cur_node);
        task_rc = processActionOpen(cur_node);
        break;
      } else if (xmlStrcmp(cur_node->name, (const xmlChar *) "close") == 0) {
        ullog_debug("action node address '%p'", cur_node);
        task_rc = processActionClose(cur_node);
        break;
      } else {
        ullog_err("node '%s' is not supported", cur_node->name);
        _xmlDump(cur_node, 0);
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

static rc_t processDecoratorNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_SUCCESS;
  xmlChar *node_type = NULL;

  if (node->type == XML_ELEMENT_NODE) {
    ullog_debug("element node '%s'", node->name);
    node_type = xmlGetProp(node, (const xmlChar *) "type");
    if (node_type && (strlen((const char *) node_type) > 0)) {
      ullog_debug("node type '%s'", node_type);
    } else {
      ullog_err("node '%s' is not supported", node->name);
      _xmlDump(node, 0);
      task_rc = RC_ERROR;
      goto bail;
    }

    if (xmlStrcmp(node_type, (const xmlChar *) "succeeder") == 0) {
      ullog_debug("decorator node type is succeeder");
      task_rc = processDecoratorSucceederNode(node);
    } else {
      ullog_err("node '%s' is not supported", node->name);
      _xmlDump(node, 0);
      task_rc = RC_ERROR;
      goto bail;
    }
  }

  bail:
  ullog_debug("task_rc %s", rc2rstr(task_rc));
  if (node_type) xmlFree(node_type);

  ullog_debug("exit");
  return(task_rc);
}

static rc_t processDecoratorSucceederNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_SUCCESS;
  xmlNodePtr cur_node = NULL;

  for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      task_rc = processNode(cur_node);
      if (task_rc == RC_FAILURE) {
        task_rc = RC_SUCCESS;
      }
      break;
    }
  }
  ullog_debug("task_rc %s", rc2rstr(task_rc));

  ullog_debug("exit");
  return(task_rc);
}

static rc_t processSequenceNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_SUCCESS;
  xmlNodePtr cur_node = NULL;

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
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

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
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

  if (xmlStrcmp(node->name, (const xmlChar *) "action") == 0) {
    ullog_debug("action node address '%p'", node);
    task_rc = processActionLeaf(node);
  } else if (xmlStrcmp(node->name, (const xmlChar *) "sequence") == 0) {
    ullog_debug("sequence node address '%p'", node);
    task_rc = processSequenceNode(node->children);
  } else if (xmlStrcmp(node->name, (const xmlChar *) "select") == 0) {
    ullog_debug("select node address '%p'", node);
    task_rc = processSelectNode(node->children);
  } else if (xmlStrcmp(node->name, (const xmlChar *) "decorator") == 0) {
    ullog_debug("decorator node address '%p'", node);
    task_rc = processDecoratorNode(node);
  } else {
    ullog_err("node '%s' is not supported", node->name);
    _xmlDump(node, 0);
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

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp(cur_node->name, (const xmlChar *) "bt") == 0) {
        ullog_debug("node is bt");
        task_rc = processSequenceNode(cur_node->children);
        // only one bt node
        goto bail;
      } else {
        ullog_err("node '%s' is not supported", cur_node->name);
        _xmlDump(cur_node, 0);
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
	ullog_deinit();

  ullog_debug("exit bte");
  return(task_rc);
}

// EOF
