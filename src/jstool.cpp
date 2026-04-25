#include "jstool.h"
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <commctrl.h>
#include <string>
#include <strsafe.h>
#include <uxtheme.h>
#include <vector>

#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "comctl32.lib")

using json = nlohmann::json;

_ETL_IMPLEMENT

// --------------------------------------------------------------------------------
// 全局配置与选项状态
// --------------------------------------------------------------------------------
struct EmJsonOptions {
  int eolMode = 0; // 0: Auto, 1: Win, 2: Unix
  bool minKeepComments = false;
  bool formatUseSpaces = true;
  int formatSpaces = 4;
  bool formatKeepIndent = false;
};
static EmJsonOptions g_options;

// --------------------------------------------------------------------------------
// 辅助工具函数
// --------------------------------------------------------------------------------

std::string utf16_to_utf8(const std::wstring &utf16Str) {
  if (utf16Str.empty())
    return std::string();
  int size_needed = WideCharToMultiByte(
      CP_UTF8, 0, &utf16Str[0], (int)utf16Str.size(), NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &utf16Str[0], (int)utf16Str.size(), &strTo[0],
                      size_needed, NULL, NULL);
  return strTo;
}

std::wstring utf8_to_utf16(const std::string &utf8Str) {
  if (utf8Str.empty())
    return std::wstring();
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0],
                                        (int)utf8Str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &utf8Str[0], (int)utf8Str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

void CleanStringW(wchar_t *sz) {
  if (!sz)
    return;
  while (*sz) {
    if (*sz == L'\r' || *sz == L'\n' || *sz == L'\t')
      *sz = L' ';
    sz++;
  }
}

// --------------------------------------------------------------------------------
// JSON5 到标准 JSON 转换 (支持裸键、单引号、尾随逗号、并去除注释防崩溃)
// --------------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------
// 核心 JSON 处理函数
// --------------------------------------------------------------------------------

void ProcessJson(HWND hwnd, int commandId) {
  try {
    UINT_PTR selSize = Editor_GetSelTextW(hwnd, 0, nullptr);
    std::wstring selectedText;
    bool allSelected = false;

    // 如果没有选择文本，则自动选择全文
    if (selSize <= 1) {
      Editor_ExecCommand(hwnd, EEID_EDIT_SELECT_ALL);
      selSize = Editor_GetSelTextW(hwnd, 0, nullptr);
      allSelected = true;
    }

    if (selSize > 1) {
      std::vector<wchar_t> buffer(selSize);
      Editor_GetSelTextW(hwnd, selSize, buffer.data());
      selectedText = buffer.data();
    }

    if (selectedText.length() < 2)
      return;

    std::string utf8Text = utf16_to_utf8(selectedText);
    std::string resultUtf8;

    size_t bomOffset = 0;
    if (utf8Text.size() >= 3 && (unsigned char)utf8Text[0] == 0xEF &&
        (unsigned char)utf8Text[1] == 0xBB &&
        (unsigned char)utf8Text[2] == 0xBF) {
      bomOffset = 3;
    }

    // 提取顶部的注释 (Min/Format 保留功能)
    std::string topComments;
    if (g_options.minKeepComments) {
      size_t i = bomOffset;
      while (i < utf8Text.length()) {
        if (isspace((unsigned char)utf8Text[i])) {
          i++;
          continue;
        }
        if (utf8Text[i] == '/' && i + 1 < utf8Text.length() &&
            utf8Text[i + 1] == '/') {
          size_t end = utf8Text.find('\n', i);
          if (end == std::string::npos)
            end = utf8Text.length();
          topComments += utf8Text.substr(i, end - i) + "\n";
          i = end;
        } else if (utf8Text[i] == '/' && i + 1 < utf8Text.length() &&
                   utf8Text[i + 1] == '*') {
          size_t end = utf8Text.find("*/", i);
          if (end == std::string::npos)
            end = utf8Text.length() - 2;
          topComments += utf8Text.substr(i, end + 2 - i) + "\n";
          i = end + 2;
        } else {
          break; // 非注释遇到结束
        }
      }
    }

    std::string strictJson = ConvertJson5ToJson(
        bomOffset > 0 ? utf8Text.substr(bomOffset) : utf8Text);

    int indentSize = g_options.formatUseSpaces ? g_options.formatSpaces : 1;
    char indentChar = g_options.formatUseSpaces ? ' ' : '\t';

    if (commandId == IDM_JSON_SORT) {
      json j = json::parse(strictJson, nullptr, true, true);
      resultUtf8 = j.dump(indentSize, indentChar);
    } else if (commandId == IDM_JSON_FORMAT) {
      nlohmann::ordered_json oj =
          nlohmann::ordered_json::parse(strictJson, nullptr, true, true);
      resultUtf8 = oj.dump(indentSize, indentChar);
    } else if (commandId == IDM_JSON_MINIFY) {
      nlohmann::ordered_json oj =
          nlohmann::ordered_json::parse(strictJson, nullptr, true, true);
      resultUtf8 = oj.dump();
    } else {
      return;
    }

    // 处理顶部注释 (仅在 Min/Format/Sort 时)
    if (g_options.minKeepComments && !topComments.empty()) {
      resultUtf8 = topComments + resultUtf8;
    }

    // 处理空行缩进剥离 (NOT recommended 默认没勾 -> 剥离空格回车)
    if (!g_options.formatKeepIndent) {
      std::string cleaned;
      cleaned.reserve(resultUtf8.size());
      for (size_t i = 0; i < resultUtf8.length(); ++i) {
        if (resultUtf8[i] == '\n') {
          // Remove trailing spaces/tabs before newline
          while (!cleaned.empty() &&
                 (cleaned.back() == ' ' || cleaned.back() == '\t' ||
                  cleaned.back() == '\r')) {
            cleaned.pop_back();
          }
        }
        cleaned += resultUtf8[i];
      }
      resultUtf8 = cleaned;
    }

    // 处理换行符 (EOL)
    std::string targetEol = "\r\n";
    if (g_options.eolMode == 1)
      targetEol = "\r\n";
    else if (g_options.eolMode == 2)
      targetEol = "\n";
    else {
      // Auto detect
      if (utf8Text.find("\r\n") != std::string::npos)
        targetEol = "\r\n";
      else if (utf8Text.find("\n") != std::string::npos)
        targetEol = "\n";
    }

    if (targetEol == "\r\n") {
      std::string eolFixed;
      eolFixed.reserve(resultUtf8.size() * 1.05);
      for (size_t i = 0; i < resultUtf8.size(); ++i) {
        if (resultUtf8[i] == '\n' && (i == 0 || resultUtf8[i - 1] != '\r')) {
          eolFixed += "\r\n";
        } else {
          eolFixed += resultUtf8[i];
        }
      }
      resultUtf8 = eolFixed;
    } else if (targetEol == "\n") {
      std::string eolFixed;
      eolFixed.reserve(resultUtf8.size());
      for (size_t i = 0; i < resultUtf8.size(); ++i) {
        if (resultUtf8[i] == '\r' && i + 1 < resultUtf8.size() &&
            resultUtf8[i + 1] == '\n') {
          continue; // Skip \r
        }
        eolFixed += resultUtf8[i];
      }
      resultUtf8 = eolFixed;
    }

    Editor_InsertW(hwnd, utf8_to_utf16(resultUtf8).c_str());

    if (allSelected) {
      POINT_PTR pt = {0, 0};
      Editor_SetCaretPos(hwnd, TRUE, &pt);
    }

    // 格式化成功后清除状态栏的错误信息
    Editor_SetStatusW(hwnd, L"JSON formatted successfully.");
  } catch (const json::parse_error &e) {
    std::wstring errorMsg = L"JSON Parse Error: ";
    errorMsg += utf8_to_utf16(e.what());
    Editor_SetStatusW(hwnd, errorMsg.c_str());
  } catch (const std::exception &e) {
    std::wstring errorMsg = L"JSTool Error: ";
    errorMsg += utf8_to_utf16(e.what());
    Editor_SetStatusW(hwnd, errorMsg.c_str());
  }
}

// --------------------------------------------------------------------------------
// 树视图实现逻辑
// --------------------------------------------------------------------------------

#define IDC_TXT_SEARCH 1002
static bool DoSearchJson(const json *current, const std::string &keyName,
                         const std::wstring &query,
                         std::vector<const json *> &path) {
  path.push_back(current);

  wchar_t buffer[1024];
  std::wstring wKey = utf8_to_utf16(keyName);

  if (current->is_object()) {
    StringCchPrintfW(buffer, 1024, L"%s { }",
                     wKey.empty() ? L"JSON" : wKey.c_str());
  } else if (current->is_array()) {
    StringCchPrintfW(buffer, 1024, L"%s [ ]",
                     wKey.empty() ? L"JSON" : wKey.c_str());
  } else {
    std::wstring wVal = utf8_to_utf16(current->dump());
    if (wKey.empty())
      StringCchPrintfW(buffer, 1024, L"%s", wVal.c_str());
    else
      StringCchPrintfW(buffer, 1024, L"%s: %s", wKey.c_str(), wVal.c_str());
  }
  CleanStringW(buffer);

  std::wstring sNode = buffer;
  for (auto &c : sNode)
    c = towlower(c);

  if (sNode.find(query) != std::wstring::npos) {
    return true;
  }

  if (current->is_object()) {
    for (auto it = current->begin(); it != current->end(); ++it) {
      if (DoSearchJson(&it.value(), it.key(), query, path))
        return true;
    }
  } else if (current->is_array()) {
    for (size_t i = 0; i < current->size(); ++i) {
      char idx[32];
      sprintf_s(idx, "[%zu]", i);
      if (DoSearchJson(&(*current)[i], idx, query, path))
        return true;
    }
  }

  path.pop_back();
  return false;
}

// 简单的搜索定位函数
void JumpToKey(HWND hwndView, const std::wstring &key) {
  if (key.empty() || key == L"JSON")
    return;

  std::wstring searchKey = key;
  size_t pos = key.find(L": ");
  if (pos != std::wstring::npos) {
    searchKey = key.substr(0, pos);
  }
  if (searchKey.empty())
    return;

  // 在编辑器中查找 "\"key\""
  std::wstring findTarget = L"\"" + searchKey + L"\"";
  Editor_FindW(hwndView,
               FLAG_FIND_CASE | FLAG_FIND_AROUND | FLAG_FIND_NO_PROMPT,
               findTarget.c_str());
}

void InsertJsonNode(HWND hTreeView, HTREEITEM hParent, const std::string &key,
                    const json &val) {
  wchar_t buffer[1024];
  std::wstring wKey = utf8_to_utf16(key);

  TVINSERTSTRUCTW tvis = {0};
  tvis.hParent = hParent;
  tvis.hInsertAfter = TVI_LAST;
  tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
  tvis.item.lParam = (LPARAM)&val;

  if (val.is_object()) {
    StringCchPrintfW(buffer, 1024, L"%s { }",
                     wKey.empty() ? L"JSON" : wKey.c_str());
    tvis.item.cChildren = val.empty() ? 0 : 1;
  } else if (val.is_array()) {
    StringCchPrintfW(buffer, 1024, L"%s [ ]",
                     wKey.empty() ? L"JSON" : wKey.c_str());
    tvis.item.cChildren = val.empty() ? 0 : 1;
  } else {
    std::wstring wVal = utf8_to_utf16(val.dump());
    if (wKey.empty())
      StringCchPrintfW(buffer, 1024, L"%s", wVal.c_str());
    else
      StringCchPrintfW(buffer, 1024, L"%s: %s", wKey.c_str(), wVal.c_str());
    tvis.item.cChildren = 0;
  }
  CleanStringW(buffer);
  tvis.item.pszText = buffer;
  TreeView_InsertItem(hTreeView, &tvis);
}

void MyCFrame::UpdateTreeView(HWND hwndView) {
  if (!m_hTreeView || !IsWindow(m_hTreeView))
    return;

  // 依赖传入的 hwndView（通常由 OnEvents/Toggle 提供），不乱猜焦点
  if (!hwndView || !IsWindow(hwndView))
    return;

  m_hWndLastView = hwndView;

  // 禁用重绘，减少闪烁
  SendMessage(m_hTreeView, WM_SETREDRAW, FALSE, 0);
  TreeView_DeleteAllItems(m_hTreeView);

  // 获取文本
  std::wstring wText;

  // 使用 EmEditor 宏函数获取每一行文本，安全且无需选择文本
  UINT_PTR nLines = Editor_GetLines(hwndView, 1); // 获取逻辑行数
  wText.reserve(nLines * 50);                     // 预分配空间

  for (UINT_PTR i = 0; i < nLines; i++) {
    GET_LINE_INFO gli = {0};
    gli.yLine = i;
    gli.flags = 1;                                            // FLAG_LOGICAL
    UINT_PTR reqSize = Editor_GetLineW(hwndView, &gli, NULL); // 包括 \0 的大小
    if (reqSize > 0) {
      std::vector<wchar_t> lineBuf(reqSize + 1, 0);
      gli.cch = reqSize + 1;
      if (Editor_GetLineW(hwndView, &gli, lineBuf.data()) > 0) {
        wText += lineBuf.data(); // 碰到 \0 会自行停止，防止 JSON 解析截断崩溃
      }
    }
    wText += L"\n";
  }

  // 清理多余空行，预防因只有回车导致解析失败（可选）
  while (!wText.empty() && (wText.back() == L'\n' || wText.back() == L'\r')) {
    wText.pop_back();
  }

  if (wText.empty()) {
    TVINSERTSTRUCTW tvis = {0};
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT;
    tvis.item.pszText = (LPWSTR)L"(Empty Document)";
    TreeView_InsertItem(m_hTreeView, &tvis);
  } else {
    try {
      std::string utf8Text = utf16_to_utf8(wText);
      size_t bomOffset = 0;
      if (utf8Text.size() >= 3 && (unsigned char)utf8Text[0] == 0xEF &&
          (unsigned char)utf8Text[1] == 0xBB &&
          (unsigned char)utf8Text[2] == 0xBF) {
        bomOffset = 3;
      }
      std::string strictJson = ConvertJson5ToJson(
          bomOffset > 0 ? utf8Text.substr(bomOffset) : utf8Text);
      m_jsonDoc = json::parse(strictJson, nullptr, false,
                              true); // Parse and store in memory
      InsertJsonNode(m_hTreeView, TVI_ROOT, "", m_jsonDoc);

      HTREEITEM hRoot = TreeView_GetRoot(m_hTreeView);
      if (hRoot)
        TreeView_Expand(m_hTreeView, hRoot, TVE_EXPAND);
    } catch (const std::exception &e) {
      TVINSERTSTRUCTW tvis = {0};
      tvis.hParent = TVI_ROOT;
      tvis.hInsertAfter = TVI_LAST;
      tvis.item.mask = TVIF_TEXT;
      std::wstring error = L"Parse Error: ";
      error += utf8_to_utf16(e.what());
      tvis.item.pszText = (LPWSTR)error.c_str();
      TreeView_InsertItem(m_hTreeView, &tvis);
    }
  }

  SendMessage(m_hTreeView, WM_SETREDRAW, TRUE, 0);
  InvalidateRect(m_hTreeView, NULL, TRUE);
}

void MyCFrame::ToggleTreeView(HWND hwndView) {
  if (m_hCustomBar && IsWindow(m_hCustomBar)) {
    Editor_CustomBarClose(hwndView, m_nBarID);
    m_hCustomBar = NULL;
    m_hTreeView = NULL;
    m_nBarID = 0;
  } else {
    static bool classRegistered = false;
    if (!classRegistered) {
      WNDCLASSEXW wcx = {0};
      wcx.cbSize = sizeof(wcx);
      wcx.lpfnWndProc = CustomBarProc;
      wcx.hInstance = EEGetInstanceHandle();
      wcx.lpszClassName = L"EmJSONTreeViewContainer";
      wcx.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1); // 使用对话框背景色
      RegisterClassExW(&wcx);
      classRegistered = true;
    }

    // 初始化公共控件
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    // 创建容器
    HWND hContainer = CreateWindowExW(
        0, L"EmJSONTreeViewContainer", L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0, 0, 200,
        400, hwndView, NULL, EEGetInstanceHandle(), NULL);

    if (!hContainer)
      return;

    m_hTreeView = CreateWindowExW(
        WS_EX_CLIENTEDGE, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_HASBUTTONS |
            TVS_LINESATROOT | TVS_SHOWSELALWAYS | WS_HSCROLL | WS_VSCROLL,
        0, 0, 200, 400, hContainer, (HMENU)IDC_TREE_VIEW, EEGetInstanceHandle(),
        NULL);

    m_hSearchBox = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT, 0, 0, 160,
        24, hContainer, (HMENU)IDC_TXT_SEARCH, EEGetInstanceHandle(), NULL);

    HWND hBtnRefresh = CreateWindowExW(
        0, L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 160, 0,
        75, 24, hContainer, (HMENU)IDC_REFRESH_BUTTON, EEGetInstanceHandle(),
        NULL);

    // 立即手动触发一次 Resize 以防 WM_SIZE 被优化略过
    RECT rcInit;
    GetClientRect(hContainer, &rcInit);
    if (m_hSearchBox)
      MoveWindow(m_hSearchBox, 0, 0, rcInit.right - 75, 24, TRUE);
    if (hBtnRefresh)
      MoveWindow(hBtnRefresh, rcInit.right - 75, 0, 75, 24, TRUE);
    if (m_hTreeView)
      MoveWindow(m_hTreeView, 0, 24, rcInit.right, rcInit.bottom - 24, TRUE);

    if (m_hTreeView) {
      // 设置现代风格主题 (Explorer 样式)
      SetWindowTheme(m_hTreeView, L"Explorer", NULL);

      // 设置界面字体
      HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
      SendMessage(m_hTreeView, WM_SETFONT, (WPARAM)hFont, TRUE);
      if (m_hSearchBox) {
        SendMessage(m_hSearchBox, WM_SETFONT, (WPARAM)hFont, TRUE);
      }

      // 增加缩进量以适应高 DPI
      TreeView_SetIndent(m_hTreeView, 20);
    }

    CUSTOM_BAR_INFO cbi = {0};
    cbi.cbSize = sizeof(cbi);
    cbi.iPos = CUSTOM_BAR_RIGHT;
    cbi.pszTitle = L"JSON TreeView";
    cbi.hwndClient = hContainer;

#ifdef _WIN64
    SetWindowLongPtrW(hContainer, GWLP_USERDATA, (LONG_PTR)this);
#else
    SetWindowLongW(hContainer, GWLP_USERDATA, (LONG)this);
#endif

    m_nBarID = Editor_CustomBarOpen(hwndView, &cbi);
    if (m_nBarID > 0) {
      m_hCustomBar = cbi.hwndCustomBar;
      // 确保容器获得焦点或立即重绘
      UpdateTreeView(hwndView);
    } else {
      DestroyWindow(hContainer);
      m_hTreeView = NULL;
    }
  }
}

// --------------------------------------------------------------------------------
// 事件处理与框架逻辑
// --------------------------------------------------------------------------------

static INT_PTR CALLBACK OptionsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                       LPARAM lParam) {
  switch (uMsg) {
  case WM_INITDIALOG:
    if (g_options.eolMode == 1)
      CheckRadioButton(hwndDlg, IDC_RDO_EOL_AUTO, IDC_RDO_EOL_UNIX,
                       IDC_RDO_EOL_WIN);
    else if (g_options.eolMode == 2)
      CheckRadioButton(hwndDlg, IDC_RDO_EOL_AUTO, IDC_RDO_EOL_UNIX,
                       IDC_RDO_EOL_UNIX);
    else
      CheckRadioButton(hwndDlg, IDC_RDO_EOL_AUTO, IDC_RDO_EOL_UNIX,
                       IDC_RDO_EOL_AUTO);

    CheckDlgButton(hwndDlg, IDC_CHK_MIN_KEEP_COMMENTS,
                   g_options.minKeepComments ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hwndDlg, IDC_CHK_FORMAT_USE_SPACES,
                   g_options.formatUseSpaces ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemInt(hwndDlg, IDC_EDT_FORMAT_SPACES, g_options.formatSpaces,
                  FALSE);
    CheckDlgButton(hwndDlg, IDC_CHK_FORMAT_KEEP_INDENT,
                   g_options.formatKeepIndent ? BST_CHECKED : BST_UNCHECKED);
    return TRUE;
  case WM_COMMAND:
    if (LOWORD(wParam) == IDOK) {
      if (IsDlgButtonChecked(hwndDlg, IDC_RDO_EOL_WIN) == BST_CHECKED)
        g_options.eolMode = 1;
      else if (IsDlgButtonChecked(hwndDlg, IDC_RDO_EOL_UNIX) == BST_CHECKED)
        g_options.eolMode = 2;
      else
        g_options.eolMode = 0;

      g_options.minKeepComments =
          (IsDlgButtonChecked(hwndDlg, IDC_CHK_MIN_KEEP_COMMENTS) ==
           BST_CHECKED);
      g_options.formatUseSpaces =
          (IsDlgButtonChecked(hwndDlg, IDC_CHK_FORMAT_USE_SPACES) ==
           BST_CHECKED);
      g_options.formatSpaces =
          GetDlgItemInt(hwndDlg, IDC_EDT_FORMAT_SPACES, NULL, FALSE);
      if (g_options.formatSpaces < 0 || g_options.formatSpaces > 32)
        g_options.formatSpaces = 4; // safety cap
      g_options.formatKeepIndent =
          (IsDlgButtonChecked(hwndDlg, IDC_CHK_FORMAT_KEEP_INDENT) ==
           BST_CHECKED);

      EndDialog(hwndDlg, IDOK);
      return TRUE;
    } else if (LOWORD(wParam) == IDCANCEL) {
      EndDialog(hwndDlg, IDCANCEL);
      return TRUE;
    }
    break;
  case WM_CLOSE:
    EndDialog(hwndDlg, IDCANCEL);
    return TRUE;
  }
  return FALSE;
}

void MyCFrame::OnCommand(HWND hwndView) {
  HMENU hMenu = CreatePopupMenu();
  AppendMenuW(hMenu, MF_STRING, IDM_JSON_FORMAT, L"JSFormat");
  AppendMenuW(hMenu, MF_STRING, IDM_JSON_SORT, L"JSSort");
  AppendMenuW(hMenu, MF_STRING, IDM_JSON_MINIFY, L"JSMin");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING, IDM_JSON_TREE, L"JSON TreeView");
  AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
  AppendMenuW(hMenu, MF_STRING, IDM_OPTIONS, L"Options");

  HBITMAP hBmpFormat =
      LoadBitmapW(EEGetInstanceHandle(), MAKEINTRESOURCEW(IDB_MENU_FORMAT));
  HBITMAP hBmpSort =
      LoadBitmapW(EEGetInstanceHandle(), MAKEINTRESOURCEW(IDB_MENU_SORT));
  HBITMAP hBmpMin =
      LoadBitmapW(EEGetInstanceHandle(), MAKEINTRESOURCEW(IDB_MENU_MINIFY));
  HBITMAP hBmpTree =
      LoadBitmapW(EEGetInstanceHandle(), MAKEINTRESOURCEW(IDB_MENU_TREE));
  HBITMAP hBmpOptions =
      LoadBitmapW(EEGetInstanceHandle(), MAKEINTRESOURCEW(IDB_MENU_OPTIONS));

  SetMenuItemBitmaps(hMenu, IDM_JSON_FORMAT, MF_BYCOMMAND, hBmpFormat,
                     hBmpFormat);
  SetMenuItemBitmaps(hMenu, IDM_JSON_SORT, MF_BYCOMMAND, hBmpSort, hBmpSort);
  SetMenuItemBitmaps(hMenu, IDM_JSON_MINIFY, MF_BYCOMMAND, hBmpMin, hBmpMin);
  SetMenuItemBitmaps(hMenu, IDM_JSON_TREE, MF_BYCOMMAND, hBmpTree, hBmpTree);
  SetMenuItemBitmaps(hMenu, IDM_OPTIONS, MF_BYCOMMAND, hBmpOptions,
                     hBmpOptions);

  POINT pt;
  GetCursorPos(&pt);
  int selId =
      TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                     pt.x, pt.y, 0, hwndView, NULL);
  DestroyMenu(hMenu);

  if (hBmpFormat)
    DeleteObject(hBmpFormat);
  if (hBmpSort)
    DeleteObject(hBmpSort);
  if (hBmpMin)
    DeleteObject(hBmpMin);
  if (hBmpTree)
    DeleteObject(hBmpTree);
  if (hBmpOptions)
    DeleteObject(hBmpOptions);

  if (selId == IDM_JSON_TREE)
    ToggleTreeView(hwndView);
  else if (selId == IDM_OPTIONS)
    DialogBoxParamW(EEGetInstanceHandle(), MAKEINTRESOURCEW(IDD_OPTIONS),
                    hwndView, OptionsDlgProc, 0);
  else if (selId > 0)
    ProcessJson(hwndView, selId);
}

void MyCFrame::OnEvents(HWND hwndView, UINT nEvent, LPARAM lParam) {
  // 不再自动重新解析大型 JSON 导致卡顿
  // 更新行为现在归属于手动点击 "Refresh" 按钮或再点击一次 "JSON TreeView" 菜单
}

LRESULT CALLBACK MyCFrame::CustomBarProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                         LPARAM lParam) {
  switch (uMsg) {

  case WM_COMMAND: {
    if (LOWORD(wParam) == IDC_REFRESH_BUTTON) {
#ifdef _WIN64
      MyCFrame *pFrame = (MyCFrame *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
#else
      MyCFrame *pFrame = (MyCFrame *)GetWindowLongW(hwnd, GWLP_USERDATA);
#endif
      if (pFrame && pFrame->m_hWndLastView) {
        pFrame->UpdateTreeView(pFrame->m_hWndLastView);
      }
      return 0;
    }

    if (LOWORD(wParam) == IDC_TXT_SEARCH && HIWORD(wParam) == 1) {
      HWND hSearch = (HWND)lParam;
      wchar_t text[256];
      GetWindowTextW(hSearch, text, 256);
      HWND hTree = GetDlgItem(hwnd, IDC_TREE_VIEW);
      if (text[0] != L'\0') {
#ifdef _WIN64
        MyCFrame *pFrame = (MyCFrame *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
#else
        MyCFrame *pFrame = (MyCFrame *)GetWindowLongW(hwnd, GWLP_USERDATA);
#endif
        if (pFrame && !pFrame->m_jsonDoc.is_null()) {
          std::wstring query = text;
          for (auto &c : query)
            c = towlower(c);

          std::vector<const json *> path;
          if (DoSearchJson(&pFrame->m_jsonDoc, "", query, path)) {
            HTREEITEM hItem = TreeView_GetRoot(hTree);
            for (size_t i = 1; i < path.size(); ++i) {
              TreeView_Expand(hTree, hItem, TVE_EXPAND);
              HTREEITEM hChild = TreeView_GetChild(hTree, hItem);
              HTREEITEM hNext = NULL;
              while (hChild) {
                TVITEMW tvi = {0};
                tvi.hItem = hChild;
                tvi.mask = TVIF_PARAM;
                if (TreeView_GetItem(hTree, &tvi)) {
                  if (tvi.lParam == (LPARAM)path[i]) {
                    hNext = hChild;
                    break;
                  }
                }
                hChild = TreeView_GetNextSibling(hTree, hChild);
              }
              if (hNext) {
                hItem = hNext;
              } else {
                break;
              }
            }
            if (hItem) {
              TreeView_SelectSetFirstVisible(hTree, hItem);
              TreeView_SelectItem(hTree, hItem);
            }
          }
        }
      }
    }
    break;
  }
  case WM_ERASEBKGND: {
    // 处理容器背景刷新，防止拖动时残影
    HDC hdc = (HDC)wParam;
    RECT rc;
    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE + 1));
    return 1;
  }
  case WM_SIZE: {
    HWND hTree = GetDlgItem(hwnd, IDC_TREE_VIEW);
    HWND hSearch = GetDlgItem(hwnd, IDC_TXT_SEARCH);
    HWND hBtn = GetDlgItem(hwnd, IDC_REFRESH_BUTTON);
    if (hTree && hSearch && hBtn) {
      RECT rc;
      GetClientRect(hwnd, &rc);
      MoveWindow(hSearch, 0, 0, rc.right - 75, 24, TRUE);
      MoveWindow(hBtn, rc.right - 75, 0, 75, 24, TRUE);
      MoveWindow(hTree, 0, 24, rc.right, rc.bottom - 24, TRUE);
    } else if (hTree) {
      RECT rc;
      GetClientRect(hwnd, &rc);
      MoveWindow(hTree, 0, 0, rc.right, rc.bottom, TRUE);
    }
    return 0;
  }
  case WM_SHOWWINDOW: {
    // 取消主动刷新，让事件系统负责
    break;
  }
  case WM_NOTIFY: {
    LPNMHDR lpnmhdr = (LPNMHDR)lParam;
    if (lpnmhdr->idFrom == IDC_TREE_VIEW) {
      if (lpnmhdr->code == TVN_ITEMEXPANDINGW) {
        LPNMTREEVIEWW pnmtv = (LPNMTREEVIEWW)lParam;
        if (pnmtv->action == TVE_EXPAND) {
          HTREEITEM hItem = pnmtv->itemNew.hItem;
          HWND hTree = lpnmhdr->hwndFrom;
          // 懒加载展开
          if (!TreeView_GetChild(hTree, hItem)) {
            const json *pVal = (const json *)pnmtv->itemNew.lParam;
            if (pVal) {
              SendMessage(hTree, WM_SETREDRAW, FALSE, 0);
              if (pVal->is_object()) {
                for (auto &el : pVal->items()) {
                  InsertJsonNode(hTree, hItem, el.key(), el.value());
                }
              } else if (pVal->is_array()) {
                for (size_t i = 0; i < pVal->size(); ++i) {
                  char idx[32];
                  sprintf_s(idx, "[%zu]", i);
                  InsertJsonNode(hTree, hItem, idx, (*pVal)[i]);
                }
              }
              SendMessage(hTree, WM_SETREDRAW, TRUE, 0);
            }
          }
        }
      } else if (lpnmhdr->code == NM_DBLCLK ||
                 lpnmhdr->code == TVN_SELCHANGEDW) {
        HWND hTree = lpnmhdr->hwndFrom;
        HTREEITEM hItem = TreeView_GetSelection(hTree);
        if (hItem) {
          TVITEMW tvi = {0};
          wchar_t textBuf[256];
          tvi.hItem = hItem;
          tvi.mask = TVIF_TEXT;
          tvi.pszText = textBuf;
          tvi.cchTextMax = 256;
          if (TreeView_GetItem(hTree, &tvi)) {
            MyCFrame *pFrame = (MyCFrame *)GetFrame(hwnd);
            if (pFrame && pFrame->m_hWndLastView) {
              JumpToKey(pFrame->m_hWndLastView, textBuf);
            }
          }
        }
      }
    }
    return 0;
  }
  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
