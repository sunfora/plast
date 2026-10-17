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
  bool     scan_resumed;
  int64_t  quote;
};

enum syntax_type: uint8_t {
  EMPTY  = 0,
  TOKEN  = 1,
  PUNCT  = 2,
  UQUOTE = 3,
  DQUOTE = 4,
  SQUOTE = 5
};

struct syntax
{
  union {
    enum syntax_type slot[8];
    uint64_t         all;
  } layers;
  uint64_t  depth;
  void     *ast;
};

enum parse_state : uint8_t {
  PARSED_NOTHING = 0,
  PARSED_OK      = 1,
  PARSED_ERROR   = 3,
  PARSED_PARTIAL = 4
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
                 struct shell_token *sent_iter,
                 struct string      *text)
{
  struct shell_token  iter_stack;
  struct shell_token *iter = &iter_stack;

  *iter = *sent_iter;
  
  int64_t iter_stop = 0;
  int64_t i         = 0;

  if (iter->stop >= text->size) {
    goto send_back_nothing;
  }

  bool quote_found = false;
  
  scan_text:
  for (i = iter->stop; i < text->size; i++)
  {
    quote_found = false;

    struct syntax *pre = &syntax[i - 1];
    struct syntax *upd = &syntax[i + 0];
    
    inherit_previous_layer:
    {
      *upd = *pre;
    }

    uint64_t pre_rest = pre->layers.all >> 8;
    uint64_t upd_rest = upd->layers.all >> 8;
    
    uint32_t pre_last_left    = __builtin_clzll(pre_rest) & (~0b111);
    uint32_t pre_last_shift   = 56 - pre_last_left;
    enum syntax_type pre_last = pre_rest >> pre_last_shift;

    update_alternating_base: 
    {
      uint8_t is_punct = (text->data[i] == ' ');
      if (is_punct && !iter->quote) {
        upd->layers.all = PUNCT;
        upd->ast        = NULL;
        upd->depth      = 0;
      } else {
        upd->layers.slot[0] = TOKEN;
      }
    }
    
    if (upd->layers.slot[0] == TOKEN)
    update_stacked_layers:
    {
      drop_layer:
      switch (pre_last) 
      {
        case DQUOTE:
        {
          if (iter->quote == 2 && text->data[i - 1] == '"') {
            upd_rest ^= (pre_last << pre_last_shift);
          }
        } break;
        case SQUOTE:
        {
          if (iter->quote == 2 && text->data[i - 1] == '\'') {
            upd_rest ^= (pre_last << pre_last_shift);
          }
        } break;
      }

      uint64_t upd_last_left  = __builtin_clzll(upd_rest) & (~0b111);
      uint64_t upd_last_shift = 64 - upd_last_left;

      insert_layer:
      {
        enum syntax_type type = EMPTY;

        if (text->data[i] != ' ') {
          type = UQUOTE;
        }
        switch (text->data[i]) {
          case '"':
          {
            type = DQUOTE;
            quote_found = true;
          } break;
          case '\'':
          {
            type = SQUOTE;
            quote_found = true;
          } break;
        }
        if (pre_last < type) {
          upd_rest |= (type << upd_last_shift);
        }
      }
      
      upd->layers.all &= 0xff;
      upd->layers.all |= (upd_rest << 8);
    }

    negotiate:
    {
      if (!iter->scan_resumed) {

        uint32_t upd_last_left    = __builtin_clzll(upd_rest) & (~0b111);
        uint32_t upd_last_shift   = 56 - upd_last_left;
        enum syntax_type upd_last = upd_rest >> upd_last_shift;
        
        if (quote_found) {
          iter->quote += 1;
          printf("%ld quote!\n", iter->quote);
        } else if (upd_last != SQUOTE && upd_last != DQUOTE) {
          iter->quote = 0;
        }
        
        uint64_t pre_type      = pre->layers.all;
        uint64_t upd_type      = upd->layers.all;
        uint8_t  pre_base_type = pre->layers.slot[0];
        uint8_t  upd_base_type = upd->layers.slot[0];

        if (pre_base_type != upd_base_type) {
          iter->stop = i;
          goto send_back_ok;
        }
        if (pre_type != upd_type) {
          iter->stop = i;
          goto send_back_partial;
        }
      }
      iter->scan_resumed = false;
    }
  }
  iter->stop = text->size; 

  send_back_ok:
  {
    if (iter->state != PARSED_PARTIAL) {
      iter->start = sent_iter->stop;
    }
    iter->state        = PARSED_OK;
    iter->size         = iter->stop - iter->start;
    iter->scan_resumed = true;
    memcpy(sent_iter, iter, sizeof(struct shell_token));
    return true;
  }

  send_back_nothing:
  {
    iter->state = PARSED_NOTHING;
    memcpy(sent_iter, iter, sizeof(struct shell_token));
    return false;
  }

  send_back_error:
  {
    iter->state   = PARSED_ERROR;
    iter->size    = iter->stop - iter->start;
    memcpy(sent_iter, iter, sizeof(struct shell_token));
    return false;
  }
  
  send_back_partial:
  {
    if (iter->state != PARSED_PARTIAL) {
      iter->start = sent_iter->stop;
    }
    iter->state        = PARSED_PARTIAL;
    iter->size         = iter->stop - iter->start;
    iter->scan_resumed = true;
    memcpy(sent_iter, iter, sizeof(struct shell_token));
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
    
    struct syntax *syntax_memory = malloc((cmd->size + 1)* sizeof(struct syntax));
    struct syntax *syntax = syntax_memory + 1;

    memset(syntax, 0, cmd->size * sizeof(struct syntax));
    
    int j = 0;
      
    struct shell_token  token_stack;
    struct shell_token *token = &token_stack;
    init_token(token);
    token->scan_resumed = true;
    
    int32_t iteration = 0;
    bool    flat      = false;

    fetch_event:
    while (next_token(syntax, token, cmd)) {
      if (syntax[token->start].layers.slot[0] == PUNCT) {
        continue;
      }
      log:
      if (1) {
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
    free(syntax_memory);
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
