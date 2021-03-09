#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

#include <boost/spirit/include/phoenix_bind.hpp>

namespace osmoh
{
  namespace parsing
  {
    week_selector_parser::week_selector_parser() : week_selector_parser::base_type(main)
    {
      using qi::uint_;
      using qi::lit;
      using qi::_1;
      using qi::_2;
      using qi::_3;
      using qi::_val;
      using osmoh::WeekRange;

      week =
          weeknum [bind(&WeekRange::SetStart, _val, _1)] >> dash >>
          weeknum [bind(&WeekRange::SetEnd, _val, _1)] >> '/' >>
          uint_ [bind(&WeekRange::SetPeriod, _val, _1)]
      |
          weeknum [bind(&WeekRange::SetStart, _val, _1)] >> dash >>
          weeknum [bind(&WeekRange::SetEnd, _val, _1)]
      |
          weeknum [bind(&WeekRange::SetStart, _val, _1)]
      ;

      main %= charset::no_case[lit("week")] >> (week % ',');
    }
  }

  bool Parse(std::string const & str, TWeekRanges & context)
  {
    return osmoh::ParseImpl<parsing::week_selector_parser>(str, context);
  }
} // namespace osmoh
