/*
 * bow.cpp
 */

// Posix
#include <ctime>
#include <cstdlib>
#include <cmath>

// IO
#include <iostream>
#include <fstream>
#include <string>

// Boost
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/program_options.hpp>

// Series
#include "YahooDriver.hpp"
#include "DaySeries.hpp"
#include "DayPrice.hpp"
#include "ReturnFactors.hpp"
#include "BOWTrader.hpp"
#include "BnHTrader.hpp"
#include "Report.hpp"

using namespace std;
using namespace boost::gregorian;

namespace po = boost::program_options;

typedef Series::DaySeries<Series::DayPrice> DB;


int main(int argc, char* argv[])
{
  string begin_date, end_date;
  string dbfile;
  unsigned entry_offset, exit_offset;
  char entry_oc, exit_oc;

  try {
	po::options_description desc("Allowed options");
	desc.add_options()
	  ("help", "produce help message")
	  ("series_file",  po::value<string>(&dbfile), "series database")
	  ("begin_date",   po::value<string>(&begin_date), "start of trading period")
	  ("end_date",     po::value<string>(&end_date), "end of trading period")
	  ("entry_offset", po::value<unsigned>(&entry_offset)->default_value(0), "offset entry from BOW (+days)")
	  ("exit_offset",  po::value<unsigned>(&exit_offset)->default_value(0), "offset exit from BOW (+days)")
	  ("entry_oc",     po::value<char>(&entry_oc)->default_value('c'), "entry open/close")
	  ("exit_oc",      po::value<char>(&exit_oc)->default_value('c'), "exit open/close")
	  ;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if( vm.count("help") ) {
	  cout << desc << endl;
	  exit(0);
	}

	if( vm["series_file"].empty() ||
		vm["begin_date"].empty() || vm["end_date"].empty() ) {
	  cout << desc << endl;
	  exit(EXIT_FAILURE);
	}

	if( entry_offset > exit_offset ) {
	  cerr << "entry_offset > exit_offset" << endl;
	  exit(EXIT_FAILURE);
	}

	if( entry_offset > 6 || exit_offset > 6 ) {
	  cerr << "entry/exit offset too large" << endl;
	  exit(EXIT_FAILURE);
	}

	cout << "Series file: " << dbfile << endl;

  } catch( exception& e ) {
	cerr << "Error: " << e.what() << endl;
	exit(EXIT_FAILURE);
  }

  Series::YahooDriver yd;
  DB db("myseries", yd);

  try {

	date load_begin(from_simple_string(begin_date));
	if( load_begin.is_not_a_date() ) {
	  cerr << "Invalid begin date " << begin_date << endl;
	  exit(EXIT_FAILURE);
	}

	date load_end(from_simple_string(end_date));
	if( load_end.is_not_a_date() ) {
	  cerr << "Invalid end date " << end_date << endl;
	  exit(EXIT_FAILURE);
	}

	cout << "Loading " << dbfile << " from " << to_simple_string(load_begin) << " to " << to_simple_string(load_end) << "..." << endl;
	if( db.load(dbfile, load_begin, load_end) <= 0 ) {
	  cerr << "No records found" << endl;
	  exit(EXIT_FAILURE);
	}

  } catch( std::out_of_range& e ) {
	cerr << "Can't get begin/end dates: " << e.what() << endl;
	exit(EXIT_FAILURE);

  } catch( Series::DriverException& e ) {
	cerr << "Driver error: " << e.what() << endl;
	exit(EXIT_FAILURE);
  }

  cout << "Records: " << db.size() << endl;
  cout << "Period: " << db.period() << endl;
  cout << "Total days: " << db.duration().days() << endl;

  BOWTrader trader(db);
  trader.run(entry_offset, entry_oc, exit_offset, exit_oc);
  trader.positions().closed().print();
  cout << "Invested days: " << trader.invested_days() << " (" << (trader.invested_days().days()/(double)db.duration().days()) * 100 << "%)" << endl;

  ReturnFactors rf(trader.positions().closed().factors(), db.duration().days(), 12);

  Report rp(rf);
  rp.print();

  // BnH
  BnHTrader bnh(db);
  bnh.run();
  ReturnFactors bnh_rf(bnh.factors(), db.duration().days(), 12);
  Report bnh_rp(bnh_rf);

  cout << endl;
  bnh_rp.roi();
  bnh_rp.cagr();
  bnh_rp.gsdm();

  return 0;
}
