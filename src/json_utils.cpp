#include "json_utils.h"
#include <cctype>

std::string ConvertJson5ToJson(const std::string &input) {
  std::string out;
  out.reserve(input.size() * 1.2);
  enum State { NORMAL, DOUBLE_STR, SINGLE_STR, LINE_COMM, BLOCK_COMM };
  State state = NORMAL;

  for (size_t i = 0; i < input.size(); ++i) {
    char c = input[i];
    switch (state) {
    case NORMAL:
      if (c == '/' && i + 1 < input.size()) {
        if (input[i + 1] == '/') {
          state = LINE_COMM;
          i++;
          continue;
        }
        if (input[i + 1] == '*') {
          state = BLOCK_COMM;
          i++;
          continue;
        }
      }
      if (c == '"') {
        state = DOUBLE_STR;
        out += c;
        continue;
      }
      if (c == '\'') {
        state = SINGLE_STR;
        out += '"';
        continue;
      }

      if (isalpha((unsigned char)c) || c == '_' || c == '$') {
        std::string word;
        size_t j = i;
        while (j < input.size() && (isalnum((unsigned char)input[j]) ||
                                    input[j] == '_' || input[j] == '$')) {
          word += input[j];
          j++;
        }
        size_t k = j;
        while (k < input.size() && isspace((unsigned char)input[k]))
          k++;
        if (k < input.size() && input[k] == ':') {
          out += '"' + word + '"';
          i = j - 1;
          continue;
        }
        out += word;
        i = j - 1;
        continue;
      }

      if (c == ',') {
        size_t j = i + 1;
        bool trailing = false;
        while (j < input.size()) {
          if (isspace((unsigned char)input[j])) {
            j++;
            continue;
          }
          if (input[j] == '/' && j + 1 < input.size() && input[j + 1] == '/') {
            while (j < input.size() && input[j] != '\n')
              j++;
            continue;
          }
          if (input[j] == '/' && j + 1 < input.size() && input[j + 1] == '*') {
            j += 2;
            while (j + 1 < input.size() &&
                   !(input[j] == '*' && input[j + 1] == '/'))
              j++;
            j += 2;
            continue;
          }
          if (input[j] == '}' || input[j] == ']') {
            trailing = true;
            break;
          }
          break;
        }
        if (trailing)
          continue;
      }
      out += c;
      break;

    case DOUBLE_STR:
      if (c == '\\') {
        out += c;
        if (i + 1 < input.size())
          out += input[++i];
      } else if (c == '"') {
        state = NORMAL;
        out += c;
      } else
        out += c;
      break;

    case SINGLE_STR:
      if (c == '\\') {
        if (i + 1 < input.size()) {
          char next = input[++i];
          if (next == '\'')
            out += '\'';
          else if (next == '"')
            out += "\\\"";
          else {
            out += '\\';
            out += next;
          }
        } else
          out += '\\';
      } else if (c == '\'') {
        state = NORMAL;
        out += '"';
      } else if (c == '"') {
        out += "\\\"";
      } else
        out += c;
      break;

    case LINE_COMM:
      if (c == '\n') {
        state = NORMAL;
        out += '\n';
      }
      break;

    case BLOCK_COMM:
      if (c == '*' && i + 1 < input.size() && input[i + 1] == '/') {
        state = NORMAL;
        i++;
      } else if (c == '\n')
        out += '\n';
      break;
    }
  }
  return out;
}
