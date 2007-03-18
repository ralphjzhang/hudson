/*
 * EOMTrader.hpp
 */

#ifndef _EOMTRADER_HPP_
#define _EOMTRADER_HPP_

// STL
#include <vector>

// Boost
#include <boost/date_time/gregorian/gregorian.hpp>

#include "DayPrice.hpp"
#include "DaySeries.hpp"
#include "Trader.hpp"


class EOMTrader: public Trader
{
  typedef Series::DaySeries<Series::DayPrice> DB;

public:
  EOMTrader(const DB& db);

  void run(unsigned entry_days, unsigned exit_days) throw(TraderException);

  boost::gregorian::date first_entry(void) { return _first_entry; }
  boost::gregorian::date last_exit(void) { return _last_exit; }
  boost::gregorian::days invested_days(void) { return _invested_days; }

private:
  const DB& _db;

  boost::gregorian::date _first_entry;
  boost::gregorian::date _last_exit;
  boost::gregorian::days _invested_days;
};

#endif // _EOMTRADER_HPP_
