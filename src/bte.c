#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlreader.h>

#include <string.h>
#include <ctype.h> // for isspace
#include <unistd.h> // for dup

static int process_node = -1;

enum  {
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

static rc_t processNode(xmlTextReaderPtr reader);

#define DEBUG 0
#if 0
#define log_debug(fmt, ...) \
        do { if (DEBUG) fprintf(stdout, "%s:%d:%s(): " fmt, __FILE__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)
#endif
#define log_debug(...) \
            do { if (DEBUG) fprintf(stdout, ##__VA_ARGS__); } while (0)

char * _trimwhitespace(char *str) {
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
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

static rc_t processActionNode(xmlTextReaderPtr reader) {
	log_debug("%s: enter\n", __FUNCTION__);
	xmlNodePtr nodeP = xmlTextReaderCurrentNode(reader);
	rc_t task_rc = RC_FAILURE;
	if (nodeP) {
    	log_debug("%s: action address %p\n", __FUNCTION__, nodeP);
    	//xmlDebugDumpNode(stdout, nodeP, -1);
	} else {
    	log_debug("%s: no node type attr\n", __FUNCTION__);
    	goto bail;
	}
	if (!xmlTextReaderHasAttributes(reader) ) {
    	log_debug("%s: no attributes\n", __FUNCTION__);
    	goto bail;
	}

	xmlChar *action_value = xmlTextReaderReadString(reader);
	if (!action_value) {
    	log_debug("%s: no action value\n", __FUNCTION__);
    	goto bail;
	}
	log_debug("%s: action value '%s'\n", __FUNCTION__, action_value);

	xmlChar *action_type_name = xmlTextReaderGetAttribute(reader, BAD_CAST "type");
	if (!action_type_name) {
    	log_debug("%s: no action type attr\n", __FUNCTION__);
    	goto bail;
	}
	log_debug("%s: action type attr '%s'\n", __FUNCTION__, action_type_name);
	if (strncmp("cmd", (const char *) action_type_name, strlen("cmd")) == 0) {
		task_rc = execCmdAction(action_value);
	}
bail:
	log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
	log_debug("%s: exit\n", __FUNCTION__);
	return task_rc;
}

static rc_t processSequenceNode(xmlTextReaderPtr reader) {
  int ret;

  log_debug("%s: enter\n", __FUNCTION__);
  xmlNodePtr nodeP = xmlTextReaderCurrentNode(reader);
  rc_t task_rc = RC_SUCCESS;
  if (nodeP) {
      log_debug("%s: seq address %p\n", __FUNCTION__, nodeP);
      //xmlDebugDumpNode(stdout, nodeP, -1);
  } else {
      log_debug("%s: no node type attr\n", __FUNCTION__);
      goto bail;
  }
  if (!xmlTextReaderHasAttributes(reader) ) {
      log_debug("%s: no attributes\n", __FUNCTION__);
      goto bail;
  }

  xmlChar *action_value = xmlTextReaderReadString(reader);
  if (!action_value) {
      log_debug("%s: no action value\n", __FUNCTION__);
      goto bail;
  }
  log_debug("%s: action value '%s'\n", __FUNCTION__, action_value);

  xmlChar *action_type_name = xmlTextReaderGetAttribute(reader, BAD_CAST "type");
  if (!action_type_name) {
      log_debug("%s: no action type attr\n", __FUNCTION__);
      goto bail;
  }
  log_debug("%s: action type attr '%s'\n", __FUNCTION__, action_type_name);

  ret = xmlTextReaderRead(reader);
  while (ret == 1) {
    task_rc = processNode(reader);
    log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
    if (task_rc != RC_SUCCESS) {
      goto bail;
    }
    ret = xmlTextReaderRead(reader);
  }

bail:
  log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
  log_debug("%s: exit\n", __FUNCTION__);
  return task_rc;
}

static rc_t processNode(xmlTextReaderPtr reader) {
    xmlChar *name, *value;
    int current_node = -1;
	rc_t task_rc = RC_SUCCESS;

	log_debug("%s: enter\n", __FUNCTION__);
    name = xmlTextReaderName(reader);
    if (name == NULL)
        name = xmlStrdup(BAD_CAST "--");
    value = xmlTextReaderValue(reader);

    log_debug("%s: element address %p depth %d type %d name %s empty %d", __FUNCTION__,
    		xmlTextReaderCurrentNode(reader),
            xmlTextReaderDepth(reader),
            xmlTextReaderNodeType(reader),
            name,
            xmlTextReaderIsEmptyElement(reader));
    if (!value) {
        if (strlen((const char *) value) > 0) {
        	//log_debug(" value '%s'", value);
        }
    }
    log_debug("\n");

  if ((xmlTextReaderNodeType(reader) == 1) && (strncmp("action", (const char *) name, strlen((const char *) name)) == 0)) {
    	log_debug("%s: action node address '%p'\n", __FUNCTION__, xmlTextReaderCurrentNode(reader));
    	process_node = 1;
    	current_node = 1;
    	task_rc = processActionNode(reader);
    	goto bail;
  } else if ((xmlTextReaderNodeType(reader) == 1) && (strncmp("sequence", (const char *) name, strlen((const char *) name)) == 0)) {
        log_debug("%s: sequence node address '%p'\n", __FUNCTION__, xmlTextReaderCurrentNode(reader));
        process_node = 1;
        current_node = 1;
        task_rc = processSequenceNode(reader);
        goto bail;
	} else {
    	current_node = -1;
	}

	//log_debug("current_node %d process_node %d\n", current_node, process_node);
    if (process_node > -1 && current_node != process_node) {
    	//log_debug("process action node\n");
    	//log_debug("process action node address '%p'\n", xmlTextReaderCurrentNode(reader));
    	//processAction(value);
    	process_node = -1;
    }

bail:
    if (name) xmlFree(name);
    if (value) xmlFree(value);
	log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
	log_debug("%s: exit\n", __FUNCTION__);
    return task_rc;
}

int streamFile(char *filename) {
    xmlTextReaderPtr reader;
    int ret;
    rc_t task_rc = RC_FAILURE;

    reader = xmlNewTextReaderFilename(filename);
    if (reader == NULL) {
        log_debug("%s: unable to open file %s\n", __FUNCTION__, filename);
        goto bail;
    }
    //xmlTextReaderNormalization(reader);

    // process root as sequence
	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		task_rc = processNode(reader);
    log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
		if (task_rc != RC_SUCCESS) {
		  goto bail;
		}
		ret = xmlTextReaderRead(reader);
	}
	if (ret != 0) {
		log_debug("%s: failed to parse file %s\n", __FUNCTION__, filename);
    goto bail;
	}

bail:
  xmlFreeTextReader(reader);
	log_debug("%s: task_rc %d\n", __FUNCTION__, task_rc);
	log_debug("%s: exit\n", __FUNCTION__);
	return task_rc;
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
        printf("provide file\n");
        return RC_ERROR;
	}
	return(streamFile(argv[1]));
}
