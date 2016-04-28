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

#define ULLOG_DEST (ULLOG_DEST_STDOUT)
//#define ULLOG_DEST (ULLOG_DEST_STDOUT | ULLOG_DEST_STDERR | ULLOG_DEST_SYSLOG)
#define ULLOG_LEVEL ULLOG_NOTICE
//#define ULLOG_LEVEL ULLOG_DEBUG
#include "../../ul/src/ullog.h"
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
};

typedef enum rc_type rc_t;

struct action_node {
  char *value;
  int type;
  int os;
};

static rc_t processFile(const char *filename);
static rc_t processRootNode(xmlNodePtr node);
static rc_t processNode(xmlNodePtr node);
static rc_t processSequenceNode(xmlNodePtr node);
static rc_t processSelectNode(xmlNodePtr node);
static rc_t processActionNode(xmlNodePtr node);

// system specific
static rc_t execCmdAction(xmlChar *value);

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

static rc_t execCmdAction(xmlChar *value) {
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
  }
  ullog_debug("task_rc %d", task_rc);

	ullog_debug("exit");
  return task_rc;
}

static rc_t processActionNode(xmlNodePtr node) {
  ullog_debug("enter");

  rc_t task_rc = RC_FAILURE;
  xmlNodePtr cur_node = NULL;
  xmlChar *action_value = NULL;

  _xmlDump(node);

  for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
    _xmlDump(cur_node);
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp(cur_node->name, (const xmlChar *) "exec") == 0) {
        ullog_debug("action node address '%p'", cur_node);
        action_value = xmlNodeGetContent(cur_node);
        if (action_value) {
          ullog_debug("action node value '%s'", action_value);
          task_rc = execCmdAction(action_value);
          break;
        }
      } else {
        ullog_err("node '%s' is not supported", cur_node->name);
        _xmlDump(cur_node);
        task_rc = RC_ERROR;
        goto bail;
      }
    }
  }

bail:

  if (action_value) xmlFree(action_value);
  ullog_debug("task_rc %d", task_rc);

  ullog_debug("exit");
  return task_rc;
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

  ullog_debug("task_rc %d", task_rc);

  ullog_debug("exit");
  return task_rc;
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

  ullog_debug("task_rc %d", task_rc);

  ullog_debug("exit");
  return task_rc;
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

  ullog_debug("task_rc %d", task_rc);

  ullog_debug("exit");
  return task_rc;
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

  ullog_debug("task_rc %d", task_rc);

  ullog_debug("exit");
  return task_rc;
}

rc_t processFile(const char *filename) {
  ullog_debug("enter");

  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL;
  rc_t task_rc = RC_FAILURE;

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
  ullog_debug("start processRootNode");
  task_rc = processRootNode(rootNode);
  ullog_debug("done processRootNode rc %d", task_rc);

bail:
  if (doc) xmlFreeDoc(doc);
  xmlCleanupParser();
  ullog_debug("task_rc %d", task_rc);

  ullog_debug("exit");
  return task_rc;
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
  ullog_debug("done processFile rc %d", task_rc);

bail:

  ullog_debug("rc %d", task_rc);
  ullog_debug("exit bte");

	ullog_deinit();
  return task_rc;
}

// EOF
