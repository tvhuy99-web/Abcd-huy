#pragma once
#ifndef CATA_SRC_A11Y_TUTORIAL_TEXT_H
#define CATA_SRC_A11Y_TUTORIAL_TEXT_H

#include <string>
#include <string_view>

namespace a11y::tutorial
{
// The full touchscreen lesson is available in the reader without replaying a
// modal popup. Store only the latest text, not a separate gameplay objective.
inline constexpr const char *last_lesson_key = "A11Y_TUTORIAL_LAST_LESSON";

inline std::string brief( const std::string &full )
{
    // Android lessons put the task at the end. Read the title and task first;
    // the actual touch instructions are available on demand in the hub.
    constexpr std::string_view task_label = "Nhiệm vụ:";
    const std::size_t task_at = full.find( task_label );
    const std::size_t title_end = full.find( ". " );
    if( task_at == std::string::npos || title_end == std::string::npos ||
        title_end >= task_at ) {
        return full;
    }
    return full.substr( 0, title_end + 1 ) + " " + full.substr( task_at ) +
           " Muốn nghe từng bước, mở Menu tổng hợp, chọn Thông tin, trình đọc và trợ giúp, " +
           "rồi chọn Nghe chi tiết bài học gần nhất.";
}
} // namespace a11y::tutorial

#endif // CATA_SRC_A11Y_TUTORIAL_TEXT_H
