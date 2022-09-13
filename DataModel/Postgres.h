/* vim:set noexpandtab tabstop=4 wrap filetype=cpp */
#ifndef Postgres_H
#define Postgres_H

#include <string>
#include <iostream>
#include <sstream>
#include <pqxx/pqxx>
#include <sys/time.h>
#include <typeinfo>
#include <cxxabi.h>  // demangle

#include "Algorithms.h" // Print for parameter packs

class Postgres {
	
	public:
	Postgres();
	~Postgres();
	void Init(std::string hostname_in="", std::string hostip_in="", int port_in=-1,
	          std::string user_in="", std::string password_in="", std::string dbname_in="");
	
	void SetVerbosity(int verb);
	// open a connection to the database
	pqxx::connection* OpenConnection(std::string* err=nullptr);
	// close the connection to the database. returns success, for whatever failure implies.
	bool CloseConnection(std::string* err=nullptr);
	
	// wrapper around exec since we handle the transaction and connection.
	// nret specifies the expected number of returned rows from the query.
	// res and row are outputs. return value is success.
	bool Query(std::string query, int nret, pqxx::result* res=nullptr, pqxx::row* row=nullptr, std::string* err=nullptr);
	
	// one that returns the value of many fields from one row as strings in a vector (row_or_col='r')
	// or the value of one field from many rows (row_or_col='c')
	bool QueryAsStrings(std::string query, std::vector<std::string> *results, char row_or_col, std::string* err=nullptr);
	
	// one that returns rows as json strings representing maps of fieldname:value
	// we uhhhhh need to check this works ok with nested strings and stuff
	bool QueryAsJsons(std::string query, std::vector<std::string> *results, std::string* err=nullptr);
	
	// ----------------- //
	// Quoting Functions //
	// ----------------- //
	// to safely handle various datatypes we need to properly quote things, which can be complex.
	// libpqxx has functions that will do this for us, but for some reason they're methods of the
	// connection or transaction classes. Since we'd like to provide users with a simple function
	// to which they can pass a std::string representing their query, we need to provide a means
	// for them to properly quote field names, table names, and values... but ideally without
	// giving out the underlying connection or transaction class objects.
	// for this we'll use a minimal 'null connection', and wrap the quoting functions.
	// TODO: there are many... though it's not clear to me which is required when...
	// https://libpqxx.readthedocs.io/en/6.3/a00255.html#ga81fe65fbb9561af7c5f0b33a9fe27e5a
	// -------
	// quote field or table names
	inline std::string pqxx_quote_name(std::string name, std::string* err=nullptr){
		if(OpenConnection(err)==nullptr) return "";
		try {
			return conn->quote_name(name);
		} catch (std::exception& e){
			std::string errmsg = std::string("Postgres::pqxx_quote_name threw exception ")
			                     +e.what()+" trying to quote '"+name+"'";
			std::cerr<<errmsg<<std::endl;
			if(err) *err=errmsg;
			return "";
		}
		return "";
	}
	// quote values
	inline std::string pqxx_quote(std::string string, std::string* err=nullptr){
		//return pqxx::nullconnection{}.quote(string);
		// annoyingly we can't use a null connection just to get libpqxx to quote things for us;
		// we must have a valid connection to a real database :/
		if(OpenConnection(err)==nullptr) return "";
		try {
			return conn->quote(string);
		} catch (std::exception& e){
			std::string errmsg = std::string("Postgres::pqxx_quote<std::string> threw exception ")
			                     +e.what()+" trying to quote '"+string+"'";
			std::cerr<<errmsg<<std::endl;
			if(err) *err=errmsg;
			return "";
		}
	}
	
	template< typename T, typename std::enable_if<std::is_arithmetic<T>::value, bool>::type potato=true >
	std::string pqxx_quote(T number, std::string* err=nullptr){
		if(OpenConnection(err)==nullptr) return "";
		try {
			return conn->quote(std::to_string(number));
		} catch (std::exception& e){
			std::string errmsg = std::string("Postgres::pqxx_quote<std::string> threw exception ")
			                     +e.what();
			std::cerr<<errmsg<<std::endl;
			if(err) *err=errmsg;
			return "";
		}
	}
	// ------------------ //
	
	bool Promote(int wait_seconds=60, std::string* err=nullptr);
	bool Demote(int wait_seconds=60, std::string* err=nullptr);
	
	private:
	int verbosity=1;
	int v_error=0;
	int v_warning=1;
	int v_message=2;
	int v_debug=3;
	std::string logmessage;
	int get_ok;
	pqxx::connection* conn=nullptr;
	
	// default connection details
	std::string dbname="";
	std::string hostaddr="";
	std::string hostname="";
	int port=-1;
	std::string dbuser="";
	std::string dbpasswd="";
	
	public:
	
//	template <typename Tuple>
//	bool ExecuteQuery(std::string query_string, std::vector<Tuple>& rets){
//		// run an SQL query and try to return the results
//		// into a vector of tuples, one entry per row.
//		// tuple contents must be compatible with the returned columns.
//		pqxx::result local_ret;
//		bool success = ExecuteQuery(query_string, 2, &local_ret, nullptr);
//		if(not success) return false; // query failed
//		
//		// XXX
//	}
	
	// specialization for a query with no returns (e.g. DELETE entry)
	bool ExecuteQuery(std::string query_string){
		// run an SQL query and try to pass the results
		// into a parameter pack. the passed arguments
		// must be compatible with the returned columns
		bool success = Query(query_string, 0, nullptr, nullptr);
		return success;
	}
	
	template <typename... Ts>
	bool ExecuteQuery(std::string query_string, Ts&&... rets){
		// run an SQL query and try to pass the results
		// into a parameter pack. the passed arguments
		// must be compatible with the returned columns
		pqxx::row local_row;
		bool success = Query(query_string, 1, nullptr, &local_row);
		if(not success) return false; // query failed
		
		success = ExpandRow<sizeof...(Ts), Ts&&...>::expand(local_row, std::forward<Ts>(rets)...);
		return success;
	}
	
	////////
	// helper function to use the contents of a pqxx::row
	// to populate a parameter pack
	template<std::size_t N, typename T, typename... Ts>
	struct ExpandRow {
		static bool expand(const pqxx::row& row, T& last, Ts&... out){
			//std::cout << __PRETTY_FUNCTION__ << "\n";
			bool ok = ExpandRow<N-1, Ts...>::expand(row, std::forward<Ts>(out)...);
			if(not ok) return false; // do not attempt further expansion.... i mean, we could...?
			try{
			//	std::cout<<"extracting argument "<<(row.size()-pqxx::row::size_type(N))<<" to variable of type "
			//	         <<type_name<decltype(last)>()<<std::endl;
				row[row.size()-pqxx::row::size_type(N)].to(last);
			}
			catch (const pqxx::sql_error &e){
				std::cerr << e.what() << std::endl
				          << "When executing query: " << e.query();
				if(e.sqlstate()!=""){
					std::cerr << ", returned with SQLSTATE error code: " << e.sqlstate();
				}
				std::cerr<<std::endl;
				std::cerr<<"Postgres::ExpandRow failed to convert sql return field "
				         <<(row.size()-pqxx::row::size_type(N))<<" to output type "
				         <<typeid(last).name()<<std::endl;
				         //<<type_name<decltype(last)>()<<std::endl;
				return false;
			}
			catch (std::exception const &e){
				std::cerr << e.what() << std::endl;
				std::cerr<<"Postgres::ExpandRow failed to convert sql return field "
				         <<(row.size()-pqxx::row::size_type(N))<<" to output type "
				         <<typeid(last).name()<<std::endl;
				         //<<type_name<decltype(last)>()<<std::endl;
				return false;
			}
			return ok;
		}
	};
	
	template<typename T>
	struct ExpandRow<1, T> {
		static bool expand(const pqxx::row& row, T& out){
			//std::cout << __PRETTY_FUNCTION__ << "\n";
			try{
				//std::cout<<"extracting last argument "<<(row.size()-1)<<" to variable of type "
				//         <<type_name<decltype(out)>()<<std::endl;
				row[row.size()-1].to(out);
			}
			catch (const pqxx::sql_error &e){
				std::cerr << e.what() << std::endl
				          << "When executing query: " << e.query();
				if(e.sqlstate()!=""){
					std::cerr << ", returned with SQLSTATE error code: " << e.sqlstate();
				}
				std::cerr<<std::endl;
				std::cerr<<"Postgres::ExpandRow failed to convert sql return field "<<(row.size()-1)
				         <<" to output type "<<typeid(out).name()<<std::endl;
				         //<<" to output type "<<type_name<decltype(out)>()<<std::endl;
				return false;
			}
			catch(std::exception const &e){
				std::cerr << e.what() << std::endl;
				std::cerr<<"Postgres::ExpandRow failed to convert sql return field "
				         <<(row.size()-1)<<" to output type "
				         <<typeid(out).name()<<std::endl;
				         //<<type_name<decltype(out)>()<<std::endl;
				return false;
			}
			return true;
		}
	};
	// end helper function
	////////
	
	
	////////
	// helper function for insertions
	template <typename... Rest>
	bool Insert(std::string tablename, std::vector<std::string> &fields, std::string* err, Rest... args){
		// maybe this is redundant since OpenConnection will check is_open (against recommendations)
		for(int tries=0; tries<2; ++tries){
			// ensure we have a connection to work with
			if(OpenConnection(err)==nullptr){
				// no connection to batabase -> abort
				return false;
			}
			std::string query_string;
			try{
				// open a transaction to interact with the database
				//pqxx::work(*conn);
				pqxx::nontransaction txn(*conn);
				
				// form the query
				query_string = "INSERT INTO " + tablename + "( ";
				for(auto&& afield : fields) query_string += afield +", ";
				query_string = query_string.substr(0, query_string.length()-2); // pop trailing comma
				query_string += ") VALUES (";
				for(int i=0; i<fields.size(); ++i) query_string += "$" + std::to_string(i+1) + ", ";
				query_string = query_string.substr(0, query_string.length()-2); // pop trailing comma
				query_string += ")";
				
				// try to run it
				txn.exec_params(query_string, args...);
				
				// don't expect (or handle) a return.
				// if we haven't thrown an exception, we're done.
				return true;
			}
			catch (const pqxx::broken_connection &e){
				// annoyingly this exception is also thrown on bad syntax for Insert
				// (and what else?) - manually check if our connection is actually ok
				std::string openerr;
				if(OpenConnection(&openerr)!=nullptr){
					// just a syntax error, not a connection error. Ugh.
					// can we repackage it as a pqxx::sql_error and re-throw? probably not easily.
					// moreover a pqxx::broken_connection object doesn't even have the same
					// members describing what the real syntax error was: all we have is e.what().
					std::stringstream errmsg;
					errmsg << e.what() << "When executing query: " << query_string << " with args: ";
					Print(errmsg, std::forward<Rest>(args)...);
					std::cerr<<errmsg.str()<<std::endl;
					if(err) *err = errmsg.str();
				} else if(tries==0){
					// our connection is broken after all: disconnect, reconnect and retry
					// however, i will note that even with a good connection this did not succeed.
					std::cerr<<"Postgres Connection error:"
					         <<" trying to close and re-open connection"<<std::endl;
					CloseConnection();
					delete conn; conn=nullptr;
					continue;
				} else {
					std::cerr<<"Postgres::Insert error - broken connection, failed to re-establish it"<<std::endl;
					if(err) *err=e.what();
				}
			}
			catch (const pqxx::sql_error &e){
				std::stringstream errmsg;
				errmsg << e.what() << "When executing query: " << e.query() << " with args: ";
				Print(errmsg, std::forward<Rest>(args)...);
				if(e.sqlstate()!=""){
					errmsg << ", returned with SQLSTATE error code: " << e.sqlstate();
				}
				std::cerr<<errmsg.str()<<std::endl;
				if(err) *err = errmsg.str();
			}
			catch (std::exception const &e){
				std::cerr << e.what() << std::endl;
				if(err) *err = e.what();
			}
			break;   // if not explicitly 'continued', break.
		} // end tries
		return false;
	}
	// end helper function
	////////
	
	////////
	// helper function for updates
	template <typename... Rest>
	// note; variadic parameter pack should contain 1 value for each field being updated,
	// followed by 1 value for each crtierion being applied.
	bool Update(std::string tablename, std::vector<std::string> &fields, std::vector<std::string> &criterions, std::vector<char> &comparators, std::string* err, Rest... args){
		// sanity check; each condition ('RunNumber > 2000') requires a field (RunNumber),
		// a comparator ('>') and a value (10).
		if(comparators.size()!=comparators.size()){
			std::string errmsg = "Postgres::Update called with criterions.size() != comparators.size()";
			if(err) *err = errmsg;
			std::cerr<<errmsg<<std::endl;
			return false;
		}
		// TODO calculate number of parameters in parameter pack and make sure it
		// adds up to fields.size() + comparators.size() 
		
		// maybe this is redundant since OpenConnection will check is_open (against recommendations)
		for(int tries=0; tries<2; ++tries){
			// ensure we have a connection to work with
			if(OpenConnection(err)==nullptr){
				// no connection to batabase -> abort
				return false;
			}
			try{
				// open a transaction to interact with the database
				//pqxx::work(*conn);
				pqxx::nontransaction txn(*conn);
				
				// form the query
				std::string query_string = "UPDATE " + tablename + " SET ";
				for(int i=0; i<fields.size(); ++i){
					std::string afield = fields.at(i);
					query_string += afield +"=$"+std::to_string(i+1);
					if(i<(fields.size()-1)) query_string+= ", ";
				}
				if(criterions.size()){
					query_string += " WHERE ";
					for(int i=0; i<criterions.size(); ++i){
						query_string += criterions.at(i)+comparators.at(i)+"$"
						             +  std::to_string(i+1+fields.size());
						if(i<(criterions.size()-1)) query_string+= ", AND ";
					}
				}
				query_string += ";";
				
				// try to run it
				txn.exec_params(query_string, args...);
				
				// don't expect (or handle) a return.
				// if we haven't thrown an exception, we're done.
				return true;
			}
			catch (const pqxx::broken_connection &e){
				// if our connection is broken after all, disconnect, reconnect and retry
				if(tries==0){
					CloseConnection();
					delete conn; conn=nullptr;
					continue;
				} else {
					std::cerr<<"Postgres::Update error - broken connection, "
					           "failed to re-establish it"<<std::endl;
					if(err) *err=e.what();
				}
			}
			catch (const pqxx::sql_error &e){
				std::stringstream errmsg;
				errmsg << e.what() << "When executing query: " << e.query() << " with args: ";
				Print(errmsg, std::forward<Rest>(args)...);
				if(e.sqlstate()!=""){
					errmsg << ", returned with SQLSTATE error code: " + e.sqlstate();
				}
				std::cerr<<errmsg.str()<<std::endl;
				if(err) *err = errmsg.str();
			}
			catch (std::exception const &e){
				std::cerr << e.what() << std::endl;
				if(err) *err = e.what();
			}
			break;   // if not explicitly 'continued', break.
		} // end tries
		return false;
	}
	// end helper function
	////////
};


#endif
