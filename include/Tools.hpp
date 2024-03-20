#ifndef FAQ_TOOLS_HPP
#define FAQ_TOOLS_HPP
#include "FAQRow.hpp"
#include "jwt-cpp/jwt.h"
#include <chrono>
#include <crow.h>
#include <cstring>
#include <iomanip>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <regex>
/*
 * Classe utilitaire.
 */
class Tools
{
  public:
    /*
     * Conversion d'une FAQRow vers une wvalue crow.
     * @param faq : la ligne.
     * @return wvalue correspondante.
     */
    static crow::json::wvalue convertToWValue(const FAQRow &faq)
    {
        crow::json::wvalue retour;

        retour["rowid"] = faq.ROWID;
        retour["question"] = faq.QUESTION;
        retour["reponse"] = faq.REPONSE;
        retour["date_ajout_question"] = faq.DATE_AJOUT_QUESTION;
        retour["date_ajout_reponse"] = faq.DATE_AJOUT_REPONSE;
        retour["reponse_valide"] = faq.REPONSE_VALIDE;
        return retour;
    }
    /*
     * Conversion de vector vers list de wvalue. S'appuie sur la méthode convertToWValue.
     * @param list : liste à convertir.
     * @return vecteur de wvalue.
     */
    template <typename T> static crow::json::wvalue convertListToWValue(const std::vector<T> &list)
    {
        std::vector<crow::json::wvalue> listValue;
        std::for_each(list.begin(), list.end(),
                      [&listValue](const T &value) { listValue.push_back(convertToWValue(value)); });

        return crow::json::wvalue(crow::json::wvalue::list(listValue));
    }
    /*
     * Méthode permettant de fournir le timestamp courant en milliseconde depuis 01/01/1970 incrémenté de n-minutes (0
     * par défaut)
     * @param addedminutes : minutes ajoutées (0 par défaut)
     * @return le timestamp.
     */
    static int64_t currentTimestamp(const std::chrono::minutes addedminutes = std::chrono::minutes(0))
    {
        auto currentTime = std::chrono::system_clock::now();
        auto finalTime = currentTime + addedminutes;
        return finalTime.time_since_epoch().count();
    }
    /*
     * Méthode de hashage d'une string vers un string avec l'algorith sha256.
     * Note: un hash est une représentation unique et non réversible d'un entrant.
     * @param input : la string à hasher.
     * @return la string hashée.
     */
    static std::string sha256(const std::string &input)
    {
        EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
        const EVP_MD *md = EVP_sha256();
        unsigned char hash[SHA256_DIGEST_LENGTH];

        EVP_DigestInit(mdctx, md);
        EVP_DigestUpdate(mdctx, input.c_str(), input.length());
        EVP_DigestFinal(mdctx, hash, nullptr);

        EVP_MD_CTX_free(mdctx);

        std::string hashedString;
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            char hex[3];
            sprintf(hex, "%02x", hash[i]);
            hashedString += hex;
        }

        return hashedString;
    }
    /*
     * Méthode de création d'un uuid à partir du timestamp courant. Un uuid et une identifiant unique.
     * @return l'uuid généré à partir du timestamp courant.
     */
    static std::string uuidFromTimestamp()
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<unsigned char> dis(0, 255);

        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();

        std::array<unsigned char, 16> uuid;
        for (int i = 0; i < 8; i++)
        {
            uuid[i] = static_cast<unsigned char>((timestamp >> ((7 - i) * 8)) & 0xFF);
        }
        for (int i = 8; i < 16; i++)
        {
            uuid[i] = dis(gen);
        }

        // Set version number
        uuid[6] &= 0x0F;
        uuid[6] |= 0x10;

        // Set variant
        uuid[8] &= 0x3F;
        uuid[8] |= 0x80;

        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        for (int i = 0; i < 16; i++)
        {
            ss << std::setw(2) << static_cast<unsigned>(uuid[i]);
            if (i == 3 || i == 5 || i == 7 || i == 9)
            {
                ss << '-';
            }
        }

        return ss.str();
    }
    /*
     * Méthode de découpage d'une string en fonction d'une regex fournie.
     * @param string : la chaine de charactère à découper.
     * @param pattern : la regex de découpage.
     * @param tokens : les éléments de la chaine découpée sont placés dans ce vecteur de string.
     */
    static void tokenizeString(const std::string &string, const std::regex &pattern, std::vector<std::string> &tokens)
    {
        tokens = std::vector<std::string>(std::sregex_token_iterator{begin(string), end(string), pattern, -1},
                                          std::sregex_token_iterator{});
    }
    /*
     * Méthode de création d'un token JWT.
     * @param iss : issuer.
     * @param scope : scope du jeton.
     * @param aud : audience du jeton.
     * @param pkey : clé privée servant à la signature du jeton.
     */
    static std::string JWTToken(const std::string &iss, const std::string &scope, const std::string &aud,
                                const std::string &pkey)
    {
        auto token = jwt::create()
                         .set_issuer(iss)
                         .set_type("JWT")
                         .set_id("rsa-create-example")
                         .set_issued_now()
                         .set_expires_in(std::chrono::seconds{1800})
                         .set_payload_claim("scope", picojson::value(scope))
                         .set_audience(aud)
                         .sign(jwt::algorithm::rs256("", pkey, "", ""));
        return token;
    }
    /*
    * Permet d'extraire un potentiel entier sous forme de string du body de la requête.
    * @param bodyParams : body de la requête.
    * @param paramName : nom du paramètre a extraire.
    * @return l'entier extrait ou si c'est impossible 0.
    */
    static unsigned int extractInteger(const crow::query_string &bodyParams, const std::string &paramName)
    {
        unsigned int retour{0};
        try
        {
            retour = std::stoi(bodyParams.get(paramName));
        }
        catch (const std::invalid_argument &e)
        {
            std::cout << paramName + " format invalid : " << e.what() << std::endl;
        }
        catch (const std::out_of_range &e)
        {
            std::cout << paramName + " format invalid : " << e.what() << std::endl;
        }
        return retour;
    }
};
#endif
