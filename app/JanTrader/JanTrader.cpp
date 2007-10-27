/*
* Copyright (C) 2007, Alberto Giannetti
*
* This file is part of Hudson.
*
* Hudson is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Hudson is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Hudson.  If not, see <http://www.gnu.org/licenses/>.
*/

// Hudson
#include <EODDB.hpp>

// Jan
#include "JanTrader.hpp"

using namespace std;
using namespace boost::gregorian;


JanTrader::JanTrader(const std::string& long_symbol,  const std::string& hedge_symbol):
  _long_symbol(long_symbol),
  _hedge_symbol(hedge_symbol),
  _vested_days(0)
{
}


void JanTrader::run(int entry_offset, int exit_offset) throw(TraderException)
{
  _vf.clear();
  _vested_days = days(0);

  const Series::EODSeries& longdb = Series::EODDB::instance().get(_long_symbol);
  const Series::EODSeries& hedgedb = Series::EODDB::instance().get(_hedge_symbol);

  // For each year
  for( year_iterator yiter( date(longdb.begin()->first.year(), 12, 20) ); yiter < longdb.rbegin()->first; ++yiter ) {

	if( (*yiter).year() == longdb.rbegin()->first.year() )
	  continue;					// we need next year Jan 9 close

	try {
	  date calendar_entry((*yiter).year(), 12, 20 + entry_offset);
	  Series::EODSeries::const_iterator long_entry_iter = longdb.lower_bound(calendar_entry);
	  Series::EODSeries::const_iterator hedge_entry_iter = hedgedb.lower_bound(calendar_entry);

	  date calendar_exit((*yiter).year()+1, 1, 9 + exit_offset);
	  Series::EODSeries::const_iterator long_exit_iter = longdb.at_or_before(calendar_exit);

	  Series::EODSeries::const_iterator hedge_exit_iter = hedgedb.at_or_before(calendar_exit);

	  if( long_entry_iter == longdb.end() ) {
		cerr << "Can't locate long trade day entry around " << calendar_entry << endl;
		continue;
	  }

	  if( hedge_entry_iter == hedgedb.end() ) {
		cerr << "Can't locate hedge trade day entry around " << calendar_entry << endl;
		continue;
	  }

	  if( long_exit_iter == longdb.end() ) {
		cerr << "Can't locate long trade day exit around " << calendar_exit << endl;
		continue;
	  }

	  if( hedge_exit_iter == hedgedb.end() ) {
		cerr << "Can't locate hedge trade day exit around " << calendar_exit << endl;
		continue;
	  }


	  Position::ID long_id = buy(longdb.name(), long_entry_iter->first, long_entry_iter->second.adjclose);
	  Position::ID hedge_id = sell_short(hedgedb.name(), hedge_entry_iter->first, hedge_entry_iter->second.adjclose);

	  close(long_id, long_exit_iter->first, long_exit_iter->second.adjclose);
	  close(hedge_id, hedge_exit_iter->first, hedge_exit_iter->second.adjclose);

	  PositionSet::const_iterator long_iter = _miPositions.find(long_id, pos_comp_id());
	  PositionSet::const_iterator hedge_iter = _miPositions.find(hedge_id, pos_comp_id());

	  if( long_iter == _miPositions.end() || hedge_iter == _miPositions.end() ) {
		cerr << "Can't find positions closed on " << long_exit_iter->first << endl;
		continue;
	  }

	  double factor = (*long_iter)->factor() * (*hedge_iter)->factor();
	  _vf.push_back(factor);

	  _vested_days = _vested_days + date_period(long_entry_iter->first, long_exit_iter->first).length();

	} catch( bad_day_of_month& me ) {
	  cerr << "Month error: " << me.what() << endl;
	  continue;

	} catch( TraderException& te ) {
	  cerr << "Trader error: " << te.what() << endl;
	  continue;

	} catch( std::exception& e ) {
	  cerr << "Standard exception: " << e.what() << endl;
	  continue;
	}
  }	// end for
}
