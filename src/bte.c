#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlreader.h>


#include <libxml/parser.h>
#include <libxml/tree.h>

#include <string.h>
#include <ctype.h> // for isspace
#include <unistd.h> // for dup

static int g_debug = 0;

enum {
  ACTION,
} node_type;

enum os_type {
  UNIX,
};
typedef enum os_type os_t;

enum rc_type {
  RC_SUCCESS, // clean success
  RC_FAILURE, // clean failure
  RC_ERROR, // unexpected failure
};

typedef enum rc_type rc_t;

struct action_node {
  char *value;
  int type;
  int os;
};

static rc_t processRootNode(xmlNode *node);
static rc_t processNode(xmlNodePtr node);
static rc_t processSequenceNode(xmlNodePtr node);
static rc_t processSelectNode(xmlNodePtr node);

#define DEBUG 0
#if 0
#define log_debug(fmt, ...) \
        do { if (DEBUG || g_debug) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#endif
#define log_debug(...) \
            do { if (DEBUG || g_debug) fprintf(stdout, ##__VA_ARGS__); } while (0)

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
  if (g_debug) {
    log_debug("%s: enter\n", __FUNCTION__);

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
    log_debug("%s: exit\n", __FUNCTION__);
  }
}

static rc_t execCmdAction(xmlChar *value) {
  log_debug("%s: enter\n", __FUNCTION__);
  rc_t task_rc = RC_FAILURE;
  int rc = 1;
  if (value) {
    _trimwhitespace((char *) value);
    log_debug("%s: executing action '%s'\n", __FUNCTION__, value);
    // TODO read stdout
    dup2(1, 2);  //redirects stderr to stdout below this line.
    rc = system((const char *) value);
    if (rc == 0) {
      task_rc = RC_SUCCESS;
    }
  }
  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

static rc_t processActionNode(xmlNodePtr node) {
  log_debug("%s: enter\n", __FUNCTION__);
  rc_t task_rc = RC_FAILURE;
  xmlNodePtr cur_node = NULL;

  xmlChar *action_value = NULL;

  _xmlDump(node);

  for (cur_node = node->children; cur_node; cur_node = cur_node->next) {
    _xmlDump(cur_node);
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp(cur_node->name, (const xmlChar *) "exec") == 0) {
        log_debug("%s: action node address '%p'\n", __FUNCTION__, cur_node);
        action_value = xmlNodeGetContent(cur_node);
        if (action_value) {
          log_debug("%s: action node value '%s'\n", __FUNCTION__, action_value);
          task_rc = execCmdAction(action_value);
          break;
        }
      } else {
        log_debug("%s: node '%s' is not supported\n", __FUNCTION__,
            cur_node->name);
        _xmlDump(cur_node);
        task_rc = RC_ERROR;
        goto bail;
      }
    }
  }

bail:

  if (action_value) xmlFree(action_value);
  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

static rc_t processSequenceNode(xmlNodePtr node) {
  log_debug("%s: enter\n", __FUNCTION__);

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

  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

static rc_t processSelectNode(xmlNodePtr node) {
  log_debug("%s: enter\n", __FUNCTION__);

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

  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

static rc_t processNode(xmlNodePtr node) {
  log_debug("%s: enter\n", __FUNCTION__);

  rc_t task_rc = RC_SUCCESS;

  _xmlDump(node);

  if (xmlStrcmp(node->name, (const xmlChar *) "action") == 0) {
    log_debug("%s: action node address '%p'\n", __FUNCTION__, node);
    task_rc = processActionNode(node);
  } else if (xmlStrcmp(node->name, (const xmlChar *) "sequence") == 0) {
    log_debug("%s: sequence node address '%p'\n", __FUNCTION__, node);
    task_rc = processSequenceNode(node->children);
  } else if (xmlStrcmp(node->name, (const xmlChar *) "select") == 0) {
    log_debug("%s: select node address '%p'\n", __FUNCTION__, node);
    task_rc = processSelectNode(node->children);
  } else {
    log_debug("%s: node '%s' is not supported\n", __FUNCTION__, node->name);
    _xmlDump(node);
    task_rc = RC_ERROR;
    goto bail;
  }

bail:

  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

static rc_t processRootNode(xmlNodePtr node) {
  log_debug("%s: enter\n", __FUNCTION__);

  rc_t task_rc = RC_SUCCESS;
  xmlNodePtr cur_node = NULL;

  //_xmlDump(node);

  for (cur_node = node; cur_node; cur_node = cur_node->next) {
    _xmlDump(cur_node);
    if (cur_node->type == XML_ELEMENT_NODE) {
      if (xmlStrcmp(cur_node->name, (const xmlChar *) "bt") == 0) {
        log_debug("%s: node is bt\n", __FUNCTION__);
        task_rc = processSequenceNode(cur_node->children);
        // only one bt node
        goto bail;
      } else {
        log_debug("%s: node '%s' is not supported\n", __FUNCTION__, cur_node->name);
        _xmlDump(cur_node);
        task_rc = RC_ERROR;
        goto bail;
      }
    }
  }

bail:

  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

int processFile(char *filename) {
  log_debug("%s: enter\n", __FUNCTION__);

  xmlDocPtr doc = NULL;
  xmlNodePtr rootNode = NULL;
  rc_t task_rc = RC_FAILURE;

  log_debug("%s: start xmlReadFile\n", __FUNCTION__);
  doc = xmlReadFile(filename, NULL, 0);
  if (doc == NULL) {
    log_debug("%s: unable to open file %s\n", __FUNCTION__, filename);
    task_rc = RC_ERROR;
    goto bail;
  }
  log_debug("%s: done xmlReadFile: doc %p\n", __FUNCTION__, doc);

  rootNode = xmlDocGetRootElement(doc);
  if (rootNode == NULL) {
    log_debug("%s: unable to open file %s: no root element\n", __FUNCTION__, filename);
    task_rc = RC_ERROR;
    goto bail;
  }
  // process root as sequence
  log_debug("%s: start processRootNode\n", __FUNCTION__);
  task_rc = processRootNode(rootNode);
  log_debug("%s: done processRootNode rc %d\n", __FUNCTION__, task_rc);

bail:
  if (doc) xmlFreeDoc(doc);
  xmlCleanupParser();
  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

int main(int argc, char *argv[]) {
  rc_t task_rc = RC_FAILURE;

  if (argc < 2) {
    printf("provide file\n");
    task_rc = RC_ERROR;
    goto bail;
  }
  if (argc == 3) {
    if (strncmp(argv[2], "-d", 2) == 0) {
      log_debug("%s: enable debug\n", __FUNCTION__);
      g_debug = 1;
    }
  }
  log_debug("%s: done process cli\n", __FUNCTION__);

  log_debug("%s: start processFile\n", __FUNCTION__);
  task_rc = processFile(argv[1]);
  log_debug("%s: done processFile rc %d\n", __FUNCTION__, task_rc);

bail:

  log_debug("%s: rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}
