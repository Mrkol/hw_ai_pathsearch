#pragma once

#include <fmt/format.h>
#include <glm/glm.hpp>


template <>
struct fmt::formatter<glm::vec2>
{
  char presentation = 'f';

  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin())
  {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

    if (it != end && *it != '}') throw format_error("invalid format");

    return it;
  }

  template <typename FormatContext>
  auto format(const glm::vec2& p, FormatContext& ctx) const -> decltype(ctx.out())
  {
    return presentation == 'f'
              ? fmt::format_to(ctx.out(), "({:.1f}, {:.1f})", p.x, p.y)
              : fmt::format_to(ctx.out(), "({:.1e}, {:.1e})", p.x, p.y);
  }
};

template <>
struct fmt::formatter<glm::ivec2>
{
  template <typename FormatContext>
  auto format(const glm::ivec2& p, FormatContext& ctx) const -> decltype(ctx.out())
  {
    return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
  }
};
