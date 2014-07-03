#include <stdio.h>
#include <stdlib.h>

#include <libxml/xmlreader.h>

#include <string.h>

static process_node = -1;

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

static rc_t execCmdAction(const char *value) {
	printf("%s: enter\n", __FUNCTION__);
	rc_t task_rc = RC_FAILURE;
	int rc = 1;
	if (value) {
    	printf("%s: action value '%s'\n", __FUNCTION__, value);
    	// TODO read stdout
    	dup2(1, 2);  //redirects stderr to stdout below this line.
    	rc = system(value);
    	if (rc == 0) {
    		task_rc = RC_SUCCESS;
    	}
	}
	printf("%s: task_rc %d\n", __FUNCTION__, task_rc);
	printf("%s: exit\n", __FUNCTION__);
	return task_rc;
}

static rc_t processActionNode(xmlTextReaderPtr reader) {
	printf("%s: enter\n", __FUNCTION__);
	xmlNodePtr nodeP = xmlTextReaderCurrentNode(reader);
	rc_t task_rc = RC_FAILURE;
	if (nodeP) {
    	printf("%s: action address %p\n", __FUNCTION__, nodeP);
    	xmlDebugDumpNode(stdout, nodeP, -1);
	} else {
    	printf("%s: no node type attr\n", __FUNCTION__);
    	goto bail;
	}
	if (!xmlTextReaderHasAttributes(reader) ) {
    	printf("%s: no attributes\n", __FUNCTION__);
    	goto bail;
	}

	char *action_value = xmlTextReaderReadString(reader);
	if (!action_value) {
    	printf("%s: no action value\n", __FUNCTION__);
    	goto bail;
	}
	printf("%s: action value '%s'\n", __FUNCTION__, action_value);

	char *action_type_name = xmlTextReaderGetAttribute(reader, "type");
	if (!action_type_name) {
    	printf("%s: no action type attr\n", __FUNCTION__);
    	goto bail;
	}
	printf("%s: action type attr '%s'\n", __FUNCTION__, action_type_name);
	if (strncmp("cmd", action_type_name, strlen("cmd")) == 0) {
		task_rc = execCmdAction(action_value);
	}
bail:
	printf("%s: task_rc %d\n", __FUNCTION__, task_rc);
	printf("%s: exit\n", __FUNCTION__);
	return task_rc;
}


static rc_t processNode(xmlTextReaderPtr reader) {
    xmlChar *name, *value;
    int current_node = -1;
	rc_t task_rc = RC_SUCCESS;

	printf("%s: enter\n", __FUNCTION__);
    name = xmlTextReaderName(reader);
    if (name == NULL)
        name = xmlStrdup(BAD_CAST "--");
    value = xmlTextReaderValue(reader);

    printf("%s: element address %p depth %d type %d name %s empty %d", __FUNCTION__,
    		xmlTextReaderCurrentNode(reader),
            xmlTextReaderDepth(reader),
            xmlTextReaderNodeType(reader),
            name,
            xmlTextReaderIsEmptyElement(reader));
    if (!value) {
        if (strlen(value) > 0) {
        	//printf(" value '%s'", value);
        }
    }
    printf("\n");

    if ((xmlTextReaderNodeType(reader) == 1) && (strncmp("action", name, strlen(name)) == 0)) {
    	printf("%s: action node address '%p'\n", __FUNCTION__, xmlTextReaderCurrentNode(reader));
    	process_node = 1;
    	current_node = 1;
    	task_rc = processActionNode(reader);
    	goto bail;
	} else {
    	current_node = -1;
	}

	//printf("current_node %d process_node %d\n", current_node, process_node);
    if (process_node > -1 && current_node != process_node) {
    	//printf("process action node\n");
    	//printf("process action node address '%p'\n", xmlTextReaderCurrentNode(reader));
    	//processAction(value);
    	process_node = -1;
    }

bail:
    if (name) xmlFree(name);
    if (value) xmlFree(value);
	printf("%s: task_rc %d\n", __FUNCTION__, task_rc);
	printf("%s: exit\n", __FUNCTION__);
    return task_rc;
}

int streamFile(char *filename) {
    xmlTextReaderPtr reader;
    int ret;
	rc_t task_rc = RC_FAILURE;

    reader = xmlNewTextReaderFilename(filename);
    if (reader == NULL) {
        printf("%s: unable to open file %s\n", __FUNCTION__, filename);
        goto bail;
    }
    //xmlTextReaderNormalization(reader);

    // process root as sequence
	ret = xmlTextReaderRead(reader);
	while (ret == 1) {
		// FIXME has to return if any node after root has failure
		task_rc = processNode(reader);
    printf("%s: task_rc %d\n", __FUNCTION__, task_rc);
		if (task_rc != RC_SUCCESS) {
		  goto bail;
		}
		ret = xmlTextReaderRead(reader);
		printf("%s: task_rc %d\n", __FUNCTION__, task_rc);
	}
	if (ret != 0) {
		printf("%s: failed to parse file %s\n", __FUNCTION__, filename);
    goto bail;
	}

bail:
  xmlFreeTextReader(reader);
	printf("%s: task_rc %d\n", __FUNCTION__, task_rc);
	printf("%s: exit\n", __FUNCTION__);
	return task_rc;
}

int main(int argc, char * argv[]) {
	if (argc != 2) {
        printf("Provide file\n");
        return RC_FAILURE;
	}
	return(streamFile(argv[1]));
}
