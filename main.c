#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>

#include "dllist.h"

#define LOG_DEBUG	1
#define LOG_INFO	2
#define LOG_ERROR	3
#define log_open(NAME)		    do { logg(LOG_INFO, "%s started", NAME } while(0);
#define log_close()		    do { logg(LOG_INFO, "stopp logging" } while(0);
#define logg(LEVEL, FMT, ARGS...)   do{ printf("%s:%s (%d): "FMT "\n", __FILE__, __FUNCTION__, __LINE__, ##ARGS); } while(0);
#define logg_err(FMT, ARGS...)      do { printf("%s:%s (%d): " FMT "\n", __FILE__, __FUNCTION__, __LINE__, ##ARGS); } while (0)

size_t
toolbox_get_file_size_fp(FILE *fp)
{
  long lEndPos;
  long curPos;

  curPos = ftell(fp);
  // Move to the end and get position
  fseek(fp, 0, SEEK_END);
  lEndPos = ftell(fp);

  //move to start position
  fseek(fp, curPos, SEEK_SET);

  return lEndPos;
}

int
toolbox_get_file_contents_locked(const char *filename, char **contents,
    size_t* length, size_t prepend_count)
{
  FILE *stream = NULL;
  size_t objects_read;
  char *buffer = NULL;
  size_t buffer_size;

  if (!filename || !contents || prepend_count < 0)
    return -1;

  *contents = NULL;
  if (length)
    *length = 0;

  stream = fopen(filename, "r");
  if (!stream)
    {
      logg_err("Cannot open %s for read.", filename);
      return -1;
    }
  if (flock(fileno(stream), LOCK_EX) < 0)
    {
      logg_err("flock failed (%s)", strerror(errno));
      goto on_error;
    }

  buffer_size = toolbox_get_file_size_fp(stream);
  if (buffer_size < 0)
    goto on_error_locked;

  buffer = (char *) malloc(buffer_size + prepend_count);
  if (!buffer)
    {
      logg_err("Cannot allocate memory.");
      goto on_error_locked;
    }

  objects_read = fread(buffer + prepend_count, buffer_size, 1, stream);
  if (objects_read <= 0)
    {
      logg_err("Failed to read file contents.");
      goto on_error_locked;
    }

  flock(fileno(stream), LOCK_UN);
  fclose(stream);

  *contents = buffer;
  if (length)
    *length = buffer_size;

  return 0;

  on_error_locked: flock(fileno(stream), LOCK_UN);
  on_error: free(buffer);
  if (stream)
    fclose(stream);
  return -1;
}

enum lease_element_value_type_t
{
  ELEMENT_VALUE_TYPE_TIME,
  ELEMENT_VALUE_TYPE_STRING,
  ELEMENT_VALUE_TYPE_INT,
};

enum lease_element_type_t
{
  LEASE_ELEMENT_TYPE_TIME_STARTS,
  LEASE_ELEMENT_TYPE_TIME_ENDS,
  LEASE_ELEMENT_TYPE_TIME_TSTP,
  LEASE_ELEMENT_TYPE_TIME_CLTT,
  LEASE_ELEMENT_TYPE_TIME_TSFP,
  LEASE_ELEMENT_TYPE_TIME_ATSFP,
  LEASE_ELEMENT_TYPE_BINDING_STATE,
  LEASE_ELEMENT_TYPE_HARDWARE,
  LEASE_ELEMENT_TYPE_NEXT_BINDING_STATE,
  LEASE_ELEMENT_TYPE_REWIND_BINDING_STATE,
  LEASE_ELEMENT_TYPE_CLIENT_HOSTNAME,
};

struct dhcpd_lease_parser
{
  const char *element_name;
  size_t element_name_size;
  int value_column;
  enum lease_element_type_t type;
  enum lease_element_value_type_t value_type;
};

#define ARRAYSIZE(n) (sizeof(n)/sizeof(n[0]))

#ifdef DEBUG
struct lease_element_type_2_str_t
{
  enum lease_element_type_t type;
  const char *str;
};

struct lease_element_type_2_str_t lease_element_type_2_str_map[] =
    {
        {.type = LEASE_ELEMENT_TYPE_TIME_STARTS,          .str = "starts",               },
        {.type = LEASE_ELEMENT_TYPE_TIME_ENDS,            .str = "ends",                 },
        {.type = LEASE_ELEMENT_TYPE_TIME_TSTP,            .str = "tstp",                 },
        {.type = LEASE_ELEMENT_TYPE_TIME_CLTT,            .str = "cltt",                 },
        {.type = LEASE_ELEMENT_TYPE_TIME_TSFP,            .str = "tsfp",                 },
        {.type = LEASE_ELEMENT_TYPE_TIME_ATSFP,           .str = "atsfp",                },
        {.type = LEASE_ELEMENT_TYPE_BINDING_STATE,        .str = "binding state",        },
        {.type = LEASE_ELEMENT_TYPE_HARDWARE,             .str = "hardware",             },
        {.type = LEASE_ELEMENT_TYPE_NEXT_BINDING_STATE,   .str = "next binding state",   },
        {.type = LEASE_ELEMENT_TYPE_REWIND_BINDING_STATE, .str = "rewind binding state", },
        {.type = LEASE_ELEMENT_TYPE_CLIENT_HOSTNAME,      .str = "client hostname",      },
    };


static const char *
lease_element_type_2_str(enum lease_element_type_t type)
{
  int i;
  for (i = 0; i < ARRAYSIZE(lease_element_type_2_str_map); i++)
    {
      if (type == lease_element_type_2_str_map[i].type)
        return lease_element_type_2_str_map[i].str;
    }

  return NULL;
}
#endif
// value_column = max. 9
struct dhcpd_lease_parser dhcp_lease_parser_map[] =
  {
      { .element_name = "  starts",               .element_name_size = sizeof("  starts") -1,               .value_column = 1, .value_type = ELEMENT_VALUE_TYPE_TIME,   .type = LEASE_ELEMENT_TYPE_TIME_STARTS},
      { .element_name = "  tstp",                 .element_name_size = sizeof("  tstp") -1,                 .value_column = 1, .value_type = ELEMENT_VALUE_TYPE_TIME,   .type = LEASE_ELEMENT_TYPE_TIME_TSTP},
      { .element_name = "  ends",                 .element_name_size = sizeof("  ends") -1,                 .value_column = 1, .value_type = ELEMENT_VALUE_TYPE_TIME,   .type = LEASE_ELEMENT_TYPE_TIME_ENDS},
      { .element_name = "  cltt",                 .element_name_size = sizeof("  cltt") -1,                 .value_column = 1, .value_type = ELEMENT_VALUE_TYPE_TIME,   .type = LEASE_ELEMENT_TYPE_TIME_CLTT},
      { .element_name = "  tsfp",                 .element_name_size = sizeof("  tsfp") -1,                 .value_column = 1, .value_type = ELEMENT_VALUE_TYPE_TIME,   .type = LEASE_ELEMENT_TYPE_TIME_TSFP},
      { .element_name = "  atsfp",                .element_name_size = sizeof("  atsfp") -1,                .value_column = 1, .value_type = ELEMENT_VALUE_TYPE_TIME,   .type = LEASE_ELEMENT_TYPE_TIME_ATSFP},
      { .element_name = "  binding state",        .element_name_size = sizeof("  binding state") -1,        .value_column = 2, .value_type = ELEMENT_VALUE_TYPE_STRING, .type = LEASE_ELEMENT_TYPE_BINDING_STATE},
      { .element_name = "  hardware",             .element_name_size = sizeof("  hardware") -1,             .value_column = 2, .value_type = ELEMENT_VALUE_TYPE_STRING, .type = LEASE_ELEMENT_TYPE_HARDWARE},
      { .element_name = "  next binding state",   .element_name_size = sizeof("  next binding state") -1,   .value_column = 3, .value_type = ELEMENT_VALUE_TYPE_STRING, .type = LEASE_ELEMENT_TYPE_NEXT_BINDING_STATE},
      { .element_name = "  rewind binding state", .element_name_size = sizeof("  rewind binding state") -1, .value_column = 3, .value_type = ELEMENT_VALUE_TYPE_STRING, .type = LEASE_ELEMENT_TYPE_REWIND_BINDING_STATE},
      { .element_name = "  client-hostname",      .element_name_size = sizeof("  client-hostname") -1,      .value_column = 1, .value_type = ELEMENT_VALUE_TYPE_STRING, .type = LEASE_ELEMENT_TYPE_CLIENT_HOSTNAME},
  };

struct lease_element_t
{
  char *ip;
  time_t starts;
  time_t ends;
  time_t tstp;
  time_t cltt;
  time_t tsfp;
  time_t atsfp;
  char *binding_state;
  char *next_binding_state;
  char *rewind_binding_state;
  char *hardware;
  char *client_hostname;
  struct dllist link;
};

void destroy_lease_element(struct lease_element_t *element)
{
  if(!element)
    return;

  free(element->ip);
  element->ip = NULL;

  free(element->binding_state);
  element->binding_state = NULL;

  free(element->next_binding_state);
  element->next_binding_state = NULL;

  free(element->rewind_binding_state);
  element->rewind_binding_state = NULL;

  free(element->hardware);
  element->hardware = NULL;

  free(element->client_hostname);
  element->client_hostname = NULL;
}

static void destroy_lease_list(struct dllist *list)
{
  struct lease_element_t *data;
  struct lease_element_t *temp;

  if(!list)
    return;

  dllist_for_each_safe(data, temp, list, link)
  {
    dllist_remove(&data->link);
    destroy_lease_element(data);
    free(data);
    data = NULL;
  }

  free(list);

}

struct dllist *
lease_parser_reade_file(char *file_path)
{
  char *lease_file_content = NULL;
  size_t lease_file_len = 0;
  char *token, *str1, *saveptr1;
  char s[3] = "\r\n;";

  struct lease_element_t *lease_element = NULL;

  enum lease_parser_state_t
  {
    LEASE_PARSER_STATE_SEARCH_ELEMENT, LEASE_PARSER_STATE_ELEMENT,
  } parser_state = LEASE_PARSER_STATE_SEARCH_ELEMENT;

  char sub_del[1] = " ";

  int j;

  struct dllist *ret_list;

  if (!file_path)
    {
      logg_err("parameter error");
      return NULL;
    }

  if (toolbox_get_file_contents_locked(file_path, &lease_file_content,
      &lease_file_len, 0) < 0)
    {
      logg_err("can't read file %s", file_path);
      return NULL;
    }

  ret_list = calloc(1, sizeof(*ret_list));

  if (!ret_list)
    goto error_end_free_lease_file;

  dllist_init(ret_list);

  for (j = 1, str1 = lease_file_content;; j++, str1 = NULL)
    {
      token = strtok_r(str1, s, &saveptr1);
      if (token == NULL)
        break;

      // skip comments
      if (token[0] == '#')
        continue;

      switch (parser_state)
        {
      case LEASE_PARSER_STATE_SEARCH_ELEMENT:
        {
          char *sub_str = NULL;
          char *sub_token, *sub_token_save;

          if (strncmp(token, "lease", sizeof("lease") - 1))
            continue;

          for (sub_str = token;; sub_str = NULL)
            {
              sub_token = strtok_r(sub_str, sub_del, &sub_token_save);
              if (sub_token == NULL)
                break;

              if (strncmp(sub_token, "lease", sizeof("lease") - 1) == 0)
                continue;

              lease_element = calloc(1, sizeof(*lease_element));

              if (!lease_element)
                {
                  logg_err("error calloc lease element");
                  goto error_end_free_dllist;
                }

              lease_element->ip = strdup(sub_token);
#ifdef DEBUG
              logg(LOG_DEBUG, "ip: %s", sub_token);
#endif
              break;
            }

          parser_state = LEASE_PARSER_STATE_ELEMENT;
        }
        break;

      case LEASE_PARSER_STATE_ELEMENT:
        {
          int i = 0, k = 0;
          char *sub_token_save, *sub_str;
          char *name;

          if (strncmp(token, "}", 1) == 0)
            {
              parser_state = LEASE_PARSER_STATE_SEARCH_ELEMENT;
              dllist_insert(ret_list, &lease_element->link);
              lease_element = NULL;
              continue;
            }

          for (i = 0; i < ARRAYSIZE(dhcp_lease_parser_map); i++)
            {
              if (!strncmp(token, dhcp_lease_parser_map[i].element_name,
                  dhcp_lease_parser_map[i].element_name_size))
                break;
            }

          if (i >= ARRAYSIZE(dhcp_lease_parser_map))
            {
#ifdef DEBUG
              logg_err("unknown: %s", token);
#endif
              break;
            }

          sub_str = token;

          for (k = 0; k <= dhcp_lease_parser_map[i].value_column; k++, sub_str =
              NULL)
            {
              name = strtok_r(sub_str, sub_del, &sub_token_save);
            }
          if (dhcp_lease_parser_map[i].value_type == ELEMENT_VALUE_TYPE_TIME)
            {
              struct tm tm;
              time_t epoch_time;
              strptime(sub_token_save, "%Y/%m/%d %H:%M:%S", &tm);
              epoch_time = mktime(&tm);
#ifdef DEBUG
              logg(LOG_DEBUG, "%s: %ld",
                  lease_element_type_2_str(dhcp_lease_parser_map[i].type), epoch_time);
#endif
              switch (dhcp_lease_parser_map[i].type)
                {
              case LEASE_ELEMENT_TYPE_TIME_STARTS:
                lease_element->starts = epoch_time;
                break;
              case LEASE_ELEMENT_TYPE_TIME_ENDS:
                lease_element->ends = epoch_time;
                break;
              case LEASE_ELEMENT_TYPE_TIME_TSTP:
                lease_element->tstp = epoch_time;
                break;
              case LEASE_ELEMENT_TYPE_TIME_CLTT:
                lease_element->cltt = epoch_time;
                break;
              case LEASE_ELEMENT_TYPE_TIME_TSFP:
                lease_element->tsfp = epoch_time;
                break;
              case LEASE_ELEMENT_TYPE_TIME_ATSFP:
                lease_element->atsfp = epoch_time;
                break;
              default:
                logg_err("unknown time element type");
                break;
                }
            }
          else
            {

              switch (dhcp_lease_parser_map[i].type)
                {
              case LEASE_ELEMENT_TYPE_BINDING_STATE:
                lease_element->binding_state = strdup(name);
                break;
              case LEASE_ELEMENT_TYPE_HARDWARE:
                lease_element->hardware = strdup(name);
                break;
              case LEASE_ELEMENT_TYPE_NEXT_BINDING_STATE:
                lease_element->next_binding_state = strdup(name);
                break;
              case LEASE_ELEMENT_TYPE_REWIND_BINDING_STATE:
                lease_element->rewind_binding_state = strdup(name);
                break;
              case LEASE_ELEMENT_TYPE_CLIENT_HOSTNAME:
                lease_element->client_hostname = strdup(name);
                break;
              default:
                logg_err("unknown element type");
                break;
                }
#ifdef DEBUG
             logg(LOG_DEBUG, "%s: %s", lease_element_type_2_str(dhcp_lease_parser_map[i].type), name);
#endif
            }
        }
        break;

      default:
        logg_err("unknown state");
        goto error_end_free_dllist;
        break;
        }
    }

  free(lease_file_content);
  return ret_list;

  error_end_free_dllist:
  destroy_lease_list(ret_list);
  error_end_free_lease_file:
  free(lease_file_content);
  return NULL;
}
#define LEASE_FILE "/var/lib/dhcp/dhcpd.leases"

int
main(int argn, char *args[])
{
  struct dllist *lease_file;
  struct lease_element_t *lease_element;

  lease_file = lease_parser_reade_file(LEASE_FILE);

  if(!lease_file)
    {
      logg_err("error parse lease file");
      return -1;
    }

  dllist_for_each(lease_element, lease_file, link)
  {
    logg(LOG_DEBUG, "ip: %s", lease_element->ip);
    logg(LOG_DEBUG, "mac: %s", lease_element->hardware);
    logg(LOG_DEBUG, "state: %s", lease_element->binding_state);
    logg(LOG_DEBUG, "client host: %s", lease_element->client_hostname);
    logg(LOG_DEBUG, "");
  }
  destroy_lease_list(lease_file);

  return 0;
}
