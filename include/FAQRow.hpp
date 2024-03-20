#ifndef FAQ_FAQROW_HPP
#define FAQ_FAQROW_HPP
#include <string>
struct FAQRow
{
    unsigned int ROWID;
    std::string QUESTION;
    std::string REPONSE;
    std::string DATE_AJOUT_QUESTION;
    std::string DATE_AJOUT_REPONSE;
    bool REPONSE_VALIDE;
};
#endif
