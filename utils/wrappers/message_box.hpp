#pragma once

#include <Windows.h>
#include <string>
#include <string_view>
#include <optional>
#include <concepts>
#include <type_traits>

template<typename T>
concept IsCharacter = std::same_as<std::remove_cv_t<T>, char> ||
std::same_as<std::remove_cv_t<T>, wchar_t>;

enum DialogResult : int {
	OK = IDOK,
	Cancel = IDCANCEL,
	Abort = IDABORT,
	Retry = IDRETRY,
	Ignore = IDIGNORE,
	Yes = IDYES,
	No = IDNO,
	TryAgain = IDTRYAGAIN,
	Continue = IDCONTINUE,
	Close = IDCANCEL
};

struct MsgBoxOptions {
	UINT flags = 0;

	// Combine options using the | operator
	friend constexpr MsgBoxOptions operator|(MsgBoxOptions lhs, MsgBoxOptions rhs) {
		return { lhs.flags | rhs.flags };
	}
};

namespace buttons {
	constexpr MsgBoxOptions OK{ MB_OK };
	constexpr MsgBoxOptions OKCancel{ MB_OKCANCEL };
	constexpr MsgBoxOptions AbortRetryIgnore{ MB_ABORTRETRYIGNORE };
	constexpr MsgBoxOptions YesNoCancel{ MB_YESNOCANCEL };
	constexpr MsgBoxOptions YesNo{ MB_YESNO };
	constexpr MsgBoxOptions RetryCancel{ MB_RETRYCANCEL };
	constexpr MsgBoxOptions CancelTryContinue{ MB_CANCELTRYCONTINUE };
	constexpr MsgBoxOptions Help{ MB_HELP };
}

namespace icons {
	constexpr MsgBoxOptions None{ 0 };
	constexpr MsgBoxOptions Error{ MB_ICONERROR };
	constexpr MsgBoxOptions Question{ MB_ICONQUESTION };
	constexpr MsgBoxOptions Warning{ MB_ICONWARNING };
	constexpr MsgBoxOptions Information{ MB_ICONINFORMATION };
	// Aliases
	constexpr MsgBoxOptions Stop{ MB_ICONSTOP };
	constexpr MsgBoxOptions Hand{ MB_ICONHAND };
	constexpr MsgBoxOptions Asterisk{ MB_ICONASTERISK };
	constexpr MsgBoxOptions Exclamation{ MB_ICONEXCLAMATION };
}

template <IsCharacter CharT>
auto show_message_box(
	std::basic_string_view<CharT> text,
	std::basic_string_view<CharT> caption,
	MsgBoxOptions options,
	HWND owner = nullptr) -> std::optional<DialogResult>
{
	const std::basic_string<CharT> safe_text{ text };
	const std::basic_string<CharT> safe_caption{ caption };

	int result = 0;

	if constexpr (std::same_as<CharT, char>) {
		result = ::MessageBoxA(owner, safe_text.c_str(), safe_caption.c_str(), options.flags);
	}
	else {
		result = ::MessageBoxW(owner, safe_text.c_str(), safe_caption.c_str(), options.flags);
	}

	if (result == 0) {
		return std::nullopt;
	}

	return static_cast<DialogResult>(result);
}