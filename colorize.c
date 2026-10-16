#include "/home/ivan/Projects/fork-open-source/make/src/gnumake.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

int plugin_is_GPL_compatible = 1;

struct shell_token 
{
  int64_t  start;
  int64_t  stop;
  int64_t  size;
  char    *content;
  int32_t  color;
  int64_t  state;
};

struct syntax
{
  union {
    uint8_t slot[8];
    int64_t all;
  } layers;
  void*   lookup;
};

enum parse_state : uint8_t {
  PARSED_NOTHING = 0,
  PARSED_OK      = 1,
  PARSED_ERROR   = 3,
  PARSED_PARTIAL = 4
};

enum syntax_type: uint8_t {
  EMPTY = 0,
  TOKEN = 1,
  PUNCT = 2
};

struct terminal_color
{
  int32_t color_bgra;
  int32_t bytes;
  char    color_ansi[56];
};

struct string
{
  const char *data;
  int64_t     size;
};

static inline 
struct string
make_string_from_cstring (const char *c_string)
{
  return (struct string) 
  {
    .data = c_string,
    .size = strlen(c_string)
  };
}

static
struct terminal_color
make_terminal_color (int32_t color_bgra)
{
  struct terminal_color result = {0};
  result.color_bgra = color_bgra;
  
  int a = (color_bgra & 0xFF000000) >> (3 * 8);
  int r = (color_bgra & 0x00FF0000) >> (2 * 8);
  int g = (color_bgra & 0x0000FF00) >> (1 * 8);
  int b = (color_bgra & 0x000000FF) >> (0 * 8);
  
  if (a) {
    result.bytes = snprintf(
      result.color_ansi, 
      sizeof(result.color_ansi),
      "\033[38;2;%d;%d;%dm",
      r, g, b
    );
  } else {
    result.bytes = snprintf(
      result.color_ansi, 
      sizeof(result.color_ansi),
      "\033[39m"
    );
  }

  return result;
}

static inline
void
init_token (struct shell_token *self)
{
  *self = (struct shell_token) {0};
}

static inline
bool token_not_initialized(struct shell_token *self)
{
  return self->start   == -1
      && self->stop    == -1
      && self->size    == 0
      && self->content == NULL;
}

static
bool next_token (struct syntax      *syntax,
                 struct shell_token *iter,
                 struct string      *text)
{
  struct shell_token  token_stack;
  struct shell_token *token = &token_stack;
  init_token(token);
  
  int64_t iter_stop = 0;
  int64_t i         = 0;
  
  iter_stop = iter->stop;

  for (i = iter_stop; i < text->size; i++)
  slide_towards_token:
  {
    if (text->data[i] != ' ') 
    parse_token:
    {

      token->start = i;
      switch (text->data[i]) 
      {
        case '"': 
        {
          i = token->start;
          parse_double_quote:
          {
            i += 1;
            for (; i < text->size; i++) {
              if (text->data[i] == '"') {
                token->stop = i + 1;
                goto send_back_partial;
              }
            }
            goto send_back_error;
          } 
        } break;
        case '\'': 
        {
          i = token->start;
          parse_single_quote:
          {
            i += 1;
            for (; i < text->size; i++) {
              if (text->data[i] == '\'') {
                token->stop = i + 1;
                goto send_back_partial;
              }
            } 
            goto send_back_error;
          }
        } break;
        default: 
        parse_unquoted:
        {
          for (i = token->start; i < text->size; i++) {
            if (text->data[i] == ' ') {
              token->stop = i;
              goto send_back_ok;
            }
            if (text->data[i] == '"') {
              token->stop = i;
              goto send_back_partial;
            }
            if (text->data[i] == '\'') {
              token->stop = i;
              goto send_back_partial;
            }
          }
          token->stop = text->size;
          goto send_back_ok;
        } 
      }
      goto send_back_error; 
    } 
    else 
    whitespace:
    {
      if (iter->state == PARSED_PARTIAL) {
        token->stop = i;
        goto send_back_ok;
      }
      continue;
    }
  }
  
  send_back_nothing:
  {
    token->state = PARSED_NOTHING;
    return false;
  }

  send_back_error:
  {
    token->state   = PARSED_ERROR;
    token->size    = token->stop - token->start;
    token->content = iter->content;
    memcpy(iter, token, sizeof(struct shell_token));
    return false;
  }
  
  send_back_partial:
  { 
    if (iter->state == PARSED_PARTIAL) {
      token->start = iter->start;
    }
    token->state   = PARSED_PARTIAL;
    token->size    = token->stop - token->start;
    token->content = iter->content;
    memcpy(iter, token, sizeof(struct shell_token));
    return true;
  }

  send_back_ok:
  {
    if (iter->state == PARSED_PARTIAL) {
      token->start = iter->start;
    }
    token->state   = PARSED_OK;
    token->size    = token->stop - token->start;
    token->content = iter->content;
    memcpy(iter, token, sizeof(struct shell_token));
    return true;
  }
}


static
char* hook (const char* command)
{
  if (strncmp(command, "clang", strlen("clang")) == 0) 
  format_clang:
  {
    struct string  cmd_stack = make_string_from_cstring(command);
    struct string *cmd       = &cmd_stack;
    int   new_size     = cmd->size * 3;
    char* new_text     = gmk_alloc(new_size);

    struct syntax* syntax = malloc(cmd->size * sizeof(struct syntax));
    memset(syntax, 0, cmd->size * sizeof(struct syntax));
    
    int j = 0;
      
    struct shell_token  token_stack;
    struct shell_token *token = &token_stack;
    init_token(token);
    
    int32_t iteration = 0;
    bool    flat      = false;

    fetch_event:
    while (next_token(syntax, token, cmd)) {
      
      log:
      if (0) {
        int buf_size = token->size + 1;
        char buf[buf_size];
        memset(buf, 0, buf_size);
        snprintf(
          buf, buf_size, 
          "%.*s", 
          (int) token->size, &cmd->data[token->start]
        );
        printf("%2.ld ", token->size);
        switch (token->state) {
          case PARSED_PARTIAL:
            printf("token | partial | ");
            break;
          case PARSED_NOTHING:
            printf("token | nothing | ");
            break;
          case PARSED_OK:
            printf("token |  ready  | ");
            break;
          case PARSED_ERROR:
            printf("token |  error  | ");
            break;
        }
        puts(buf);
      }

      if (token->state == PARSED_PARTIAL) {
        continue;
      }

      if (iteration > 0) {
        new_text[j++] = ' ';
        if (!flat) {
          int32_t color;
          struct terminal_color fmt;
          color = 0xFF555555;
          
          fmt = make_terminal_color(color);
          memcpy(&new_text[j], fmt.color_ansi, fmt.bytes);
          j += fmt.bytes;
          new_text[j++] = '\\';
          fmt = make_terminal_color(0);
          memcpy(&new_text[j], fmt.color_ansi, fmt.bytes);
          j += fmt.bytes;

          new_text[j++] = '\n';

          if (iteration >= 1) {
            new_text[j++] = ' ';
            new_text[j++] = ' ';
          }
        }
      }

      write_token:
      {
        const void *src  = &cmd->data[token->start];
        int64_t     size = token->size;

        select_color:
        if (strncmp("-I", &cmd->data[token->start], 2) == 0) {
          token->color = 0xFFEAF8CE;
        } else if (strncmp("clang", &cmd->data[token->start], 5) == 0) {
          token->color = 0xFFBDDA57;
        } else if (strncmp("-f", &cmd->data[token->start], 2) == 0) {
          token->color = 0xFFAAFFAA;
        } else if (strncmp("-o", &cmd->data[token->start], 2) == 0) {
          token->color = 0xFFAAFFAA;
        } else if (strncmp("-l", &cmd->data[token->start], 2) == 0) {
          token->color = 0xFF7BEACE;
        } else if (strncmp("-L", &cmd->data[token->start], 2) == 0) {
          token->color = 0xFFCEF8E7;
        } else if (strncmp("-shared", &cmd->data[token->start], strlen("-shared")) == 0) {
          token->color = 0xFF7BEACE;
        } else {
          token->color = 0xFFffffff;
        }

        if (token->color) {
          struct terminal_color fmt = make_terminal_color(token->color);
          memcpy(&new_text[j], fmt.color_ansi, fmt.bytes);
          j += fmt.bytes;
        }
        memcpy(&new_text[j], src, size);
        j += size;
        if (token->color) {
          struct terminal_color fmt = make_terminal_color(0);
          memcpy(&new_text[j], fmt.color_ansi, fmt.bytes);
          j += fmt.bytes;
        }
      }


      flat = strncmp("-o", &cmd->data[token->start], token->size) == 0;
      iteration += 1;
    }
    
    return new_text;
  } else {
    return NULL; 
  }
}

int colorize_gmk_setup ()
{
  gmk_add_hook("format_command", (gmk_hook_ptr) &hook);
  return 1;
}
