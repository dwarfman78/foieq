#ifndef FAQ_DATAACCESS_HPP
#define FAQ_DATAACCESS_HPP
#include "FAQRow.hpp"
#include "IDataAccess.hpp"
#include "SQLiteCpp/SQLiteCpp.h"
#include <iostream>
#include <optional>
#include <vector>
class SqliteDataAccess : public IDataAccess
{
  public:
    SqliteDataAccess(const std::string &database) : mDatabase(database)
    {
    }
    bool createQuestion(const std::string &question)
    {
        std::cout << "CREATE "
                  << " Question : " << question << std::endl;
        try
        {
            // Open a database file
            SQLite::Database db(mDatabase, SQLite::OPEN_READWRITE);

            SQLite::Statement query(db, "INSERT INTO FAQ VALUES (?,NULL,DATETIME(),NULL,0)");

            query.bind(1, question);
            query.exec();
            return true;
        }
        catch (std::exception &e)
        {
            std::cout << "exception: " << e.what() << std::endl;
        }
        return false;
    }
    bool updateQuestion(int rowid, const std::string &reponse, bool reponse_valide)
    {
        std::cout << "UPDATE " << rowid << " Reponse : " << reponse << " valide : " << reponse_valide << std::endl;
        try
        {
            // Open a database file
            SQLite::Database db(mDatabase, SQLite::OPEN_READWRITE);

            SQLite::Statement query(
                db, "UPDATE FAQ SET REPONSE=?, REPONSE_VALIDE=?, DATE_AJOUT_REPONSE=datetime() WHERE ROWID=?");

            query.bind(1, reponse);
            query.bind(2, reponse_valide ? 1 : 0);
            query.bind(3, rowid);
            query.exec();
            return true;
        }
        catch (std::exception &e)
        {
            std::cout << "exception: " << e.what() << std::endl;
        }
        return false;
    }
    bool deleteQuestion(int rowid)
    {
        std::cout << "DELETE" << std::endl;
        try
        {
            // Open a database file
            SQLite::Database db(mDatabase, SQLite::OPEN_READWRITE);

            // Compile a SQL query, containing one parameter (index 1)
            SQLite::Statement query(db, "DELETE FROM FAQ WHERE ROWID=(?)");

            query.bind(1, rowid);
            query.exec();
            return true;
        }
        catch (std::exception &e)
        {
            std::cout << "exception: " << e.what() << std::endl;
        }
        return false;
    }
    std::optional<std::vector<FAQRow>> getAllValidated()
    {
        return fetchAndMapResults("SELECT ROWID,QUESTION,REPONSE,DATE_AJOUT_QUESTION,DATE_AJOUT_REPONSE,REPONSE_VALIDE "
                                  "FROM FAQ WHERE REPONSE_VALIDE = 1");
    }
    std::optional<std::vector<FAQRow>> getAll()
    {
        return fetchAndMapResults(
            "SELECT ROWID,QUESTION,REPONSE,DATE_AJOUT_QUESTION,DATE_AJOUT_REPONSE,REPONSE_VALIDE FROM FAQ");
    }

  private:
    std::optional<std::vector<FAQRow>> fetchAndMapResults(const std::string &request)
    {

        try
        {
            // Open a database file
            SQLite::Database db(mDatabase);

            // Compile a SQL query, containing one parameter (index 1)
            SQLite::Statement query(db, request);

            std::vector<FAQRow> retourRequete;
            // Loop to execute the query step by step, to get rows of result
            while (query.executeStep())
            {
                FAQRow row;

                row.ROWID = query.getColumn(0);
                row.QUESTION = std::string(query.getColumn(1));
                row.REPONSE = std::string(query.getColumn(2));
                row.DATE_AJOUT_QUESTION = std::string(query.getColumn(3));
                row.DATE_AJOUT_REPONSE = std::string(query.getColumn(4));
                row.REPONSE_VALIDE = (unsigned int)query.getColumn(5) == 1 ? true : false;
                retourRequete.push_back(row);
            }
            return {retourRequete};
        }
        catch (std::exception &e)
        {
            std::cout << "exception: " << e.what() << std::endl;
        }
        return std::nullopt;
    }
    std::string mDatabase;
};
#endif
