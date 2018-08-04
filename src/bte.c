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
static int g_expect_debug = 0;
#define SELECT_SLEEP_USEC 200000

enum {
    ACTION,
} node_type;

enum os_type {
    UNIX,
};
typedef enum os_type os_t;

// return code 
typedef enum {
    RC_SUCCESS, // clean success
    RC_FAILURE, // clean failure
    RC_RUNNING, // running
    RC_ERROR, // unexpected failure
    RC_UNKNOWN, // unknown code
} rc_t;

typedef struct {
    const int code;
    const char *str;
} rcs_t;

const rcs_t rcs_str_mapping[] = {
    {RC_SUCCESS, "success"},
    {RC_FAILURE, "failure"},
    {RC_RUNNING, "running"},
    {RC_ERROR, "error"},
    {RC_UNKNOWN, NULL},
};

struct action_node {
    char *value;
    int type;
    int os;
};

#define STREAM_CHUNK_SIZE 512
#define STREAM_BUF_SIZE 2048
typedef struct {
    const char * id; // node id
    FILE * fp; 
    int fd;
    fd_set fds;
    char read_buf[STREAM_BUF_SIZE];
    size_t read_bytes;
    char write_buf[STREAM_BUF_SIZE];
    size_t written_bytes;
    UT_hash_handle hh; /* makes this structure hashable */
} fp_table_t;
static fp_table_t * fp_table = NULL;

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
static rc_t processActionExpect(xmlNodePtr node);
static rc_t processActionWrite(xmlNodePtr node);


static int 
nodeSetState(xmlNodePtr node, rc_t state_rc) 
{
    if(!xmlSetProp(node, (const xmlChar *) "_state_", 
        (const xmlChar *) rcs_str_mapping[state_rc].str)) {
        ullog_err("cannot write node state '%s' to tree", 
                 rcs_str_mapping[state_rc].str);
        return -1;
    }
    return 0;
}

static const char * 
rc2rstr(const int rc) {
    int i = 0;
    for (; rcs_str_mapping[i].code != RC_UNKNOWN; ++i) {
        if(rcs_str_mapping[i].code == rc) {
            return(rcs_str_mapping[i].str);
        }
    }
    return NULL;
}

static int rand_init = 0;
static char * 
_gen_node_id(void)
{
    char *id_str = NULL; 
    if((id_str = malloc(255)) == NULL) { 
        return NULL; 
    }
    if(!rand_init) {
        rand_init = 1;
        srand(time(NULL));
    }
    if (sprintf(id_str, "%d", rand()) == -1) {
        return NULL;
    }
    return id_str;
}

#if 0
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
#endif

static void 
_xmlDump(xmlNode *node, int recursive) 
{
  if (node) {
      printf("source line: %d\n", node->line);
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

/**
 * \brief   write asynchronously chunk of data   
 * \return:
 *  N - number of bytes written
 *  -1 - error, error number is in errno 
 *  -2 - receiver is nor ready, write again
 */
int
async_write_chunk(int fd, char * buf, size_t len)
{
    int n = 0;
    int tn = 0;
    int i = 0;
    int esc = 0;
    char ch[STREAM_CHUNK_SIZE] = "";
    char esc_ch[1] = "";

    ullog_debug("async_write_chunk: buf '%s' len %zu", buf, len);
    strcpy(ch, buf);
    errno = 0;
    while(ch[i]) {
        ullog_debug("async_write_chunk: esc %d i %d ch %c", esc, i, ch[i]);
        if(ch[i] == '\\') {
            esc = 1; ++i; ++tn; continue; 
        }
        if(esc) {
            esc = 0;
            if(ch[i] == 'n') {
                esc_ch[0] = '\n';
                n = write(fd, &esc_ch, 1);
            }
        } else {
            n = write(fd, &ch[i], 1);
        }
        ullog_debug("async_write_chunk: n %d buf '%s' errno %d err '%s'",
            n, &ch[i], errno, strerror(errno));
        ++i;
        if(n == -1 && errno == EAGAIN) {
            ullog_debug("async_write_chunk: repeat write");
            return -2;
        }
        tn += n;
    }
    ullog_debug("async_write_chunk: finish n %d buf '%s' errno %d err '%s'",
            tn, &ch[i], errno, strerror(errno));
    return tn;
}

static rc_t 
processActionExec(xmlNodePtr node) 
{
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
            //_trimwhitespace((char *) action_value);
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
        if(nodeSetState(node, RC_SUCCESS)) {
            ullog_err("cannot write node state to tree");
            task_rc = RC_ERROR;
            goto bail;
        }
    } else {
        ullog_debug("no exec action output eof");
        task_rc = RC_RUNNING;
        if (action_state) {
            if(nodeSetState(node, RC_RUNNING)) {
                ullog_err("cannot write node state to tree");
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
    if (node_id && (task_rc != RC_RUNNING)) xmlFree(node_id);
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
    return task_rc;
}

static rc_t 
processActionOpen(xmlNodePtr node) 
{
    ullog_debug("enter");

    rc_t task_rc = RC_FAILURE;
    xmlChar *node_id = NULL;
    xmlChar *stream_id = NULL;
    xmlChar *action_value = NULL;
    xmlChar *action_state = NULL;
    fp_table_t *fp_table_item = NULL;
    // enable to print hash
    // fp_table_t *fp_table_item_tmp = NULL;
    int opt = 0;
    char **argv = NULL;
    int argc = 0;
    char *argvcp = NULL;
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
            //_trimwhitespace((char *) action_value);
            if (strlen((const char *) action_value) == 0) {
                ullog_err("cannot read command value or it is empty");
                task_rc = RC_ERROR;
                goto bail;
            }

            ullog_debug("create store fp item");
            fp_table_item = (fp_table_t *) calloc(1, sizeof(fp_table_t));
            if(!fp_table_item) {
                ullog_err("cannot create fp table item");
                task_rc = RC_ERROR;
                goto bail;
            }


            fp_table_item->written_bytes = 0;
#if 0
            //fp_table_item->write_buf[0] = '\0';
            fp_table_item->write_buf = (char *) calloc(1, STREAM_BUF_SIZE);
            if(!fp_table_item->write_buf) {
                ullog_err("cannot create fp table write buf");
                task_rc = RC_ERROR;
                goto bail;
            }
#endif

            ullog_debug("action value '%s'", action_value);
            argvcp = strdup((const char *) action_value);
            token = strtok(argvcp, delim);
            while (token != NULL) {
                argv = (char **) realloc(argv, (argc + 1) * sizeof(char *));
                argv[argc] = (char *) calloc(255, sizeof(char));
                snprintf(argv[argc], 255, "%s", token);
                token = strtok(NULL, delim);
                ++argc;
            }
            argv = (char **) realloc(argv, (argc + 1) * sizeof(char *));
            argv[argc] = (char *) NULL;

            if(g_expect_debug) {
                exp_is_debugging = 1;
                exp_loguser = 1;
                exp_timeout = 1; // return immediately
            }

            if(!(fp_table_item->fd = exp_spawnv(argv[0], (char **) argv))) {
                ullog_err("cannot execute command '%s'", action_value);
                task_rc = RC_FAILURE;
                goto bail;
            }
            if(fp_table_item->fd < 1) {
                ullog_err("cannot open stream for command '%s'", action_value);
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
            FD_ZERO(&(fp_table_item->fds));
            FD_SET(fp_table_item->fd, &(fp_table_item->fds));

            task_rc = RC_SUCCESS;

            ullog_debug("start store fp in fp table");
            fp_table_item->id = (const char *) stream_id;
            ullog_debug("id '%s'", fp_table_item->id);
            ullog_debug("fp '%p'", fp_table_item->fp);
            ullog_debug("fd '%d'", fp_table_item->fd);
            HASH_ADD_KEYPTR(hh, fp_table, fp_table_item->id, 
                    strlen(fp_table_item->id), fp_table_item);
            ullog_debug("done store fp in fp table");
            if(nodeSetState(node, RC_SUCCESS)) {
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
    //if (node_id && (task_rc != RC_RUNNING)) xmlFree(node_id);
    //if (stream_id && (task_rc != RC_RUNNING)) xmlFree(stream_id);
    if (action_value) xmlFree(action_value);
    if (action_state) xmlFree(action_state);
    if(fp_table && fp_table_item) {
        if(task_rc == RC_ERROR) {
            if(fp_table_item->fd) close(fp_table_item->fd);
            HASH_DEL(fp_table, fp_table_item);
            free(fp_table_item);
        }
    }
    if(argc && argv) {
        for (i = 0; i < argc; ++i) {
            if(argv[i]) free(argv[i]);
        }
        free(argv);
    }

    ullog_debug("exit");
    return task_rc;
}

static rc_t 
processActionClose(xmlNodePtr node) 
{
    ullog_debug("enter");

    rc_t task_rc = RC_FAILURE;
    xmlChar *node_id = NULL;
    xmlChar *stream_id = NULL;
    xmlChar *action_value = NULL;
    xmlChar *action_state = NULL;
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
                FD_ZERO(&(fp_table_item->fds));
            }

            HASH_DEL(fp_table, fp_table_item);
            free(fp_table_item);
            if(nodeSetState(node, RC_SUCCESS)) {
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
    if (node_id && (task_rc != RC_RUNNING)) xmlFree(node_id);
    if (stream_id && (task_rc != RC_RUNNING)) xmlFree(stream_id);
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
    return task_rc;
}

static rc_t 
processActionExpect(xmlNodePtr node) 
{
    ullog_debug("enter");

    rc_t task_rc = RC_FAILURE;
    xmlChar *node_id = NULL;
    xmlChar *stream_id = NULL;
    xmlChar *action_value = NULL;
    xmlChar *action_state = NULL;
    fp_table_t *fp_table_item = NULL;
    // enable to print hash
    // fp_table_t *fp_table_item_tmp = NULL;
    struct timeval tv;
    int rc = 0;
    int select_rc = -1;

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
    }

    action_value = xmlNodeGetContent(node);
    if (action_value) {
        //_trimwhitespace((char *) action_value);
        if (strlen((const char *) action_value) == 0) {
            ullog_err("cannot read command value or it is empty");
            task_rc = RC_ERROR;
            goto bail;
        }
        ullog_debug("action value '%s'", action_value);

        if(!fp_table_item) {
            ullog_debug("search fp_table_item");
            HASH_FIND_STR(fp_table, (const char *) stream_id, fp_table_item);
        }
        ullog_debug("stream_id p '%p'", stream_id);
        ullog_debug("stream_id '%s'", stream_id);
        ullog_debug("fp_table '%p'", fp_table);
        ullog_debug("fp_table_item '%p'", fp_table_item);
        if(!fp_table_item) {
            ullog_err("cannot find open stream id for node id '%s'", node_id);
            task_rc = RC_ERROR;
            goto bail;
        }
        if(fp_table_item->fd < 1) {
            ullog_err("stream id is not opened for node id '%s'", node_id);
            task_rc = RC_FAILURE;
            goto bail;
        }

        // wait for data for reading
        // no blocking
        tv.tv_sec = 0;
        tv.tv_usec = SELECT_SLEEP_USEC;
        errno = 0;
        select_rc = select(fp_table_item->fd + 1, &(fp_table_item->fds), NULL, NULL, &tv);
        if(select_rc == -1) {
            ullog_debug("stream id '%s' select error error '%s'", 
                    node_id, strerror(errno));
            task_rc = RC_FAILURE;
            goto bail;
        } else if (select_rc == 0) {
            ullog_debug("stream id '%s' select timeout", node_id);
            if(nodeSetState(node, RC_RUNNING)) {
                ullog_err("cannot write node state to tree");
                task_rc = RC_ERROR;
                goto bail;
            }
            task_rc = RC_RUNNING;
            goto bail;
        } 
        ullog_debug("start reading stream id '%s'", node_id);

        // do actual action
        errno = 0;
        ullog_debug("df '%d' exp_regexp '%d' action_value '%s' exp_end %d", 
                fp_table_item->fd, exp_regexp, action_value, exp_end);
        rc = exp_expectl(fp_table_item->fd, exp_glob, action_value, 1, 
                exp_end);
        ullog_debug("rc '%d' error '%s'", rc, strerror(errno));
        //ullog_debug("rc '%d' buffer '%s' matched '%s' error '%s'", 
        //    rc, exp_buffer, exp_match, strerror(errno));
        if(rc == 1) {
            ullog_debug("MATCHED");
            if(nodeSetState(node, RC_SUCCESS)) {
                ullog_err("cannot write node state to tree");
                task_rc = RC_ERROR;
                goto bail;
            }
            task_rc = RC_SUCCESS;
        } else if(rc == EXP_ABEOF) {
            if(errno == EAGAIN) {
                ullog_debug("not ready: keep running");
                if(nodeSetState(node, RC_RUNNING)) {
                    ullog_err("cannot write node state to tree");
                    task_rc = RC_ERROR;
                    goto bail;
                }
                task_rc = RC_RUNNING;
            } else {
                ullog_err("stream I/O error: %s", strerror(errno));
                task_rc = RC_FAILURE;
            }
#if 0
        } else if(rc == EXP_TIMEOUT) {
            ullog_debug("expect timeout: keep running");
            if(nodeSetState(node, RC_RUNNING)) {
                ullog_err("cannot write node state to tree");
                task_rc = RC_ERROR;
                goto bail;
            }
            task_rc = RC_RUNNING;
#endif
        } else {
            ullog_debug("not matched error: %s", strerror(errno));
            task_rc = RC_FAILURE;
        }


    } else {
        ullog_err("cannot read command value or it is empty");
        task_rc = RC_ERROR;
        goto bail;
    }

    if(g_debug) sleep(1);

#if 0
  // list fps in table for debugging only
  HASH_ITER(hh, fp_table, fp_table_item, fp_table_item_tmp) {
   	ullog_debug("id '%s'", fp_table_item->id);
   	ullog_debug("fp '%p'", fp_table_item->fp);
  }
#endif


    bail:
    ullog_debug("task_rc %s", rc2rstr(task_rc));
    //if (node_id && (task_rc != RC_RUNNING)) xmlFree(node_id);
    //if (stream_id&& (task_rc != RC_RUNNING) ) xmlFree(stream_id);
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
    return task_rc;
}

static rc_t 
processActionWrite(xmlNodePtr node) 
{
    ullog_debug("enter");

    rc_t task_rc = RC_FAILURE;
    xmlChar *node_id = NULL;
    xmlChar *stream_id = NULL;
    xmlChar *action_value = NULL;
    xmlChar *action_state = NULL;
    fp_table_t *fp_table_item = NULL;
    // enable to print hash
    // fp_table_t *fp_table_item_tmp = NULL;
    //int rc = 0;
    int n = 0;
    int select_rc = -1;
    struct timeval tv;

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
            //_trimwhitespace((char *) action_value); 
            if (strlen((const char *) action_value) == 0) {
                ullog_err("cannot read command value or it is empty");
                task_rc = RC_ERROR;
                goto bail;
            }

#if 0            
            ullog_debug("create store fp item");
            fp_table_item = (fp_table_t *) malloc(sizeof(fp_table_t));
            if(!fp_table_item) {
                ullog_err("cannot create fp table item");
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
#endif
            if(!fp_table_item) {
                ullog_debug("search fp_table_item");
                HASH_FIND_STR(fp_table, (const char *) stream_id, fp_table_item);
            }
            ullog_debug("node_id '%p'", node_id);
            ullog_debug("stream_id '%p'", stream_id);
            ullog_debug("fp_table '%p'", fp_table);
            ullog_debug("fp_table_item '%p'", fp_table_item);
            if(!fp_table_item) {
                ullog_err("cannot find open fp for node id '%s'", node_id);
                task_rc = RC_ERROR;
                goto bail;
            }

            ullog_debug("set action value '%s'", action_value);
            memset(fp_table_item->write_buf, 0, STREAM_BUF_SIZE);
            snprintf(fp_table_item->write_buf, STREAM_BUF_SIZE, "%s", 
                    (const char *) action_value);
            //strcpy(fp_table_item->write_buf, (const char *) action_value);
            //strcpy(fp_table_item->write_buf, "pwd");

        } else {
            ullog_err("cannot read command value or it is empty");
            task_rc = RC_ERROR;
            goto bail;
        }
    }


    if(!fp_table_item) {
        ullog_debug("search fp_table_item");
        HASH_FIND_STR(fp_table, (const char *) stream_id, fp_table_item);
    }
    if(!fp_table_item) {
        ullog_err("cannot find open stream id for node id '%s'", node_id);
        task_rc = RC_ERROR;
        goto bail;
    }
    if(fp_table_item->fd < 1) {
        ullog_err("stream id is not opened for node id '%s'", node_id);
        task_rc = RC_FAILURE;
        goto bail;
    }
    ullog_debug("stream_id '%p'", stream_id);
    ullog_debug("fp_table '%p'", fp_table);
    ullog_debug("fp_table_item '%p'", fp_table_item);
    ullog_debug("write buf '%s'", fp_table_item->write_buf);

    // wait for data for writing
    // no blocking
    tv.tv_sec = 0;
    tv.tv_usec = SELECT_SLEEP_USEC;
    errno = 0;
    select_rc = select(fp_table_item->fd + 1, NULL, &(fp_table_item->fds), 
            NULL, &tv);
    if(select_rc == -1) {
        ullog_debug("stream id '%s' select error error '%s'",
                node_id, strerror(errno));
        task_rc = RC_FAILURE;
        goto bail;
    } else if (select_rc == 0) {
        ullog_debug("stream id '%s' select timeout", node_id);
        if(nodeSetState(node, RC_RUNNING)) {
            ullog_err("cannot write node state to tree");
            task_rc = RC_ERROR;
            goto bail;
        }
        task_rc = RC_RUNNING;
        goto bail;
    }
    ullog_debug("start writing to stream id '%s'", node_id);

    // start do actual action
    n = async_write_chunk(fp_table_item->fd, 
        fp_table_item->write_buf + fp_table_item->written_bytes, 
        (strlen(fp_table_item->write_buf) > STREAM_CHUNK_SIZE) ? 
        STREAM_CHUNK_SIZE : strlen(fp_table_item->write_buf));
    //ullog_debug("async_write: n %d chunk '%s' buf '%s' total written %d", 
    //        n, fp_table_item->write_buf[fp_table_item->written_bytes], 
    //        fp_table_item->write_buf, fp_table_item->written_bytes);

    if (n > 0) {
        ullog_debug("async_write: got chunk of written");
        fp_table_item->written_bytes += n;
        task_rc = RC_RUNNING;
        if(g_debug) sleep(1);
    } else if (n == -1) {
        ullog_err("async_write: error writing chunk");
        fp_table_item->written_bytes = -1;
        task_rc = RC_FAILURE;
    } else if (n == -2) {
        ullog_debug("async_write: keep writting chunk\n");
    }
    if (fp_table_item->written_bytes >= strlen(fp_table_item->write_buf)) {
        fp_table_item->written_bytes = 0;
        ullog_debug("async_write: finished writing buffer\n");
        task_rc = RC_SUCCESS;
    }
    // finish do actual action


    bail:
    ullog_debug("task_rc %s", rc2rstr(task_rc));
    //if (node_id && (task_rc != RC_RUNNING)) xmlFree(node_id);
    //if (stream_id&& (task_rc != RC_RUNNING) ) xmlFree(stream_id);
    if (action_value) xmlFree(action_value);
    if (action_state) xmlFree(action_state);
    if(fp_table && fp_table_item) {
        if(task_rc != RC_RUNNING) {
            //HASH_DEL(fp_table, fp_table_item);
            //free(fp_table_item);
        }
    }
    if(nodeSetState(node, task_rc)) {
        ullog_err("cannot write node state to tree");
        task_rc = RC_ERROR;
    }

    ullog_debug("exit");
    return task_rc;
}

static rc_t
processActionLeaf(xmlNodePtr node)
{
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
            } else if (xmlStrcmp(cur_node->name, (const xmlChar *) "expect") == 0) {
                ullog_debug("action node address '%p'", cur_node);
                task_rc = processActionExpect(cur_node);
                break;
            } else if (xmlStrcmp(cur_node->name, (const xmlChar *) "write") == 0) {
                ullog_debug("action node address '%p'", cur_node);
                task_rc = processActionWrite(cur_node);
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
    return task_rc;
}

static rc_t 
processDecoratorNode(xmlNodePtr node) 
{
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
    return task_rc;
}

static rc_t 
processDecoratorSucceederNode(xmlNodePtr node)
{
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
    return task_rc;
}

static rc_t 
processSequenceNode(xmlNodePtr node)
{
    ullog_debug("enter");

    rc_t task_rc = RC_SUCCESS;
    xmlNodePtr cur_node = NULL;

    for (cur_node = node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            task_rc = processNode(cur_node);
            if (task_rc == RC_FAILURE || task_rc == RC_ERROR || task_rc == RC_RUNNING) {
                goto bail;
            }
        }
    }

    bail:
    ullog_debug("task_rc %s", rc2rstr(task_rc));

    ullog_debug("exit");
    return task_rc;
}

static rc_t 
processSelectNode(xmlNodePtr node) 
{
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
    return task_rc;
}

static rc_t 
processNode(xmlNodePtr node) 
{
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
    return task_rc;
}

static rc_t 
processRootNode(xmlNodePtr node) 
{
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
    return task_rc;
}

rc_t 
processFile(const char *filename) 
{
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
    return task_rc;
}

int 
main(int argc, char *argv[])
{
    ullog_init("bte");
    ullog_debug("enter bte");

    rc_t task_rc = RC_FAILURE;

    ullog_debug("enable debug");

    if (argc < 2) {
        ullog_err("provide file");
        task_rc = RC_ERROR;
        goto bail;
    }
    if (argc == 3) {
        if (strncmp(argv[1], "-d", 2) == 0) {
            ullog_debug("enable debug");
            g_debug = 1;
        }
    }
    ullog_debug("done process cli");

    ullog_debug("start processFile");
    if (argc == 3) {
        task_rc = processFile(argv[2]);
    } else {
        task_rc = processFile(argv[1]);
    }
    ullog_debug("done processFile rc %s", rc2rstr(task_rc));

    bail:
    ullog_debug("rc %s", rc2rstr(task_rc));
    ullog_deinit();

    ullog_debug("exit bte");
    return task_rc;
}

// EOF
