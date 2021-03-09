#include "parse_opening_hours.hpp"
#include "opening_hours_parsers.hpp"

#include <boost/spirit/include/phoenix_bind.hpp>

namespace osmoh
{
  namespace parsing
  {
    year_selector_parser::year_selector_parser() : year_selector_parser::base_type(main)
    {
      using qi::uint_;
      using qi::lit;
      using qi::_1;
      using qi::_2;
      using qi::_3;
      using qi::_val;
      using osmoh::YearRange;

      static const qi::int_parser<unsigned, 10, 4, 4> year = {};

      year_range =
          year [bind(&YearRange::SetStart, _val, _1)] >> dash >>
          year [bind(&YearRange::SetEnd, _val, _1)] >> '/' >>
          uint_ [bind(&YearRange::SetPeriod, _val, _1)]
      |
          year [bind(&YearRange::SetStart, _val, _1)] >> dash >>
          year [bind(&YearRange::SetEnd, _val, _1)]
      |
          year [bind(&YearRange::SetStart, _val, _1)] >> lit('+') [bind(&YearRange::SetPlus, _val, true)]
      ;

      main %= (year_range % ',');
    }
  }

  bool Parse(std::string const & str, TYearRanges & context)
  {
    return osmoh::ParseImpl<parsing::year_selector_parser>(str, context);
  }
} // namespace osmoh
