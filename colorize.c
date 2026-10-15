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
struct shell_token
init_token (struct shell_token *self)
{
  *self = (struct shell_token) {
    .start   = -1,
    .stop    = -1,
    .size    = 0,
    .content = NULL,
    .color   = 0x0  
  };
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
bool next_token (struct shell_token *iter,
                 struct string      *text)
{
  struct shell_token  token_stack;
  struct shell_token *token = &token_stack;
  init_token(token);
  
  int64_t iter_stop = 0;
  int64_t i         = 0;

  if (!token_not_initialized(iter)) {
    iter_stop = iter->stop;
  }

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
        parse_double_quote:
        {
          for (i = token->start; i < text->size; i++) {
            if (text->data[i] == '"') {
              token->stop = i;
              goto send_back_ok;
            }
          } 
          goto send_back_error;
        } 
        case '\'': 
        parse_single_quote:
        {
          for (i = token->start; i < text->size; i++) {
            if (text->data[i] == '\'') {
              token->stop = i;
              goto send_back_ok;
            }
          }
          goto send_back_error;
        }
        default: 
        parse_unquoted:
        {
          for (i = token->start; i < text->size; i++) {
            if (text->data[i] == ' ') {
              token->stop = i;
              goto send_back_ok;
            }
          }
          token->stop = text->size;
          goto send_back_ok;
        } 
      }
      goto send_back_error; 
    } 
    else 
    skip_whitespace:
    {
      continue;
    }
  }
  
  send_back_nothing:
  {
    return false;
  }

  send_back_error:
  {
    return false;
  }

  send_back_ok:
  {
    token->size    = token->stop - token->start;
    token->content = iter->content;
    memcpy(iter, token, sizeof(struct shell_token));
    return true;
  }
}

static
char* hook (const char* command)
{
  if (strncmp(command, "clang", strlen("clang")) == 0) {
    struct string  cmd_stack = make_string_from_cstring(command);
    struct string *cmd       = &cmd_stack;
    int   new_size     = cmd->size * 3;
    char* new_text     = gmk_alloc(new_size);
    
    int j = 0;
      
    struct shell_token  token_stack;
    struct shell_token *token = &token_stack;
    init_token(token);
    
    int32_t iteration = 0;
    bool    flat      = false;
    while (next_token(token, cmd)) {
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

      // new_text[j++] = '`'; 
      {
        const void *src  = &cmd->data[token->start];
        int64_t     size = token->size;
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
      // new_text[j++] = '`';


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
