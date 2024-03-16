#ifndef FAQ_GOOGLESHEETDATAACCESS_HPP
#define FAQ_GOOGLESHEETDATAACCESS_HPP
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "IDataAccess.hpp"
#include "Tools.hpp"
#include "cpp-httplib/httplib.h"
#include "json/json.hpp"
#include <iostream>
using json = nlohmann::json;
/*
 * Classe d'accès aux données s'appuyant sur google sheets (excel).
 */
class GoogleSheetDataAccess : public IDataAccess
{
  public:
    /*
     * Constructeur.
     * @param pSpreadsheetId : identifiant de la feuille excel.
     * @param pApiKey : API_KEY permettant l'accès en lecture à la feuille.
     * @param pTab : Nom de la feuille (Feuille 1, Sheet1 ...)
     * @param pPrivateKey : clé privée permettant de signer le jeton JWT d'accès OAUTH2 en écriture.
     * @param pServiceAccount : Adresse mail du service account permettant l'accès en écriture au document.
     * @param pFields : champ de cellules de mise à jour (A1:G1)
     */
    GoogleSheetDataAccess(const std::string &pSpreadsheetId, const std::string &pApiKey, const std::string &pTab,
                          const std::string &pPrivateKey, const std::string &pServiceAccount,
                          const std::string &pFields)
        : mSpreadsheetId(pSpreadsheetId), mApiKey(pApiKey), mTab(pTab), mPrivateKey(pPrivateKey),
          mServiceAccount(pServiceAccount), mFields(pFields)
    {
    }

    virtual bool createQuestion(const std::string &question, unsigned int numQuestion = 0)
    {
        // Mise à jour SI NECESSAIRE du jeton d'accès.
        //  Le jeton généré est valable 30 minutes on évite donc de le renouveller si ce n'est
        //  pas nécessaire.
        majAccessToken();

        // Nouvelle question au format JSON.
        json nouvelleQuestion;
        // on spécifie la range (tab+fields) en supprimant le formattage HTML des espaces (%20) éventuels.
        nouvelleQuestion["range"] = std::regex_replace(mTab, std::regex("%20"), " ") + "!" + mFields;
        // on spếcifie que l'on va fournir une ligne entière.
        nouvelleQuestion["majorDimension"] = "ROWS";
        // la ligne de la nouvelle question.
        nouvelleQuestion["values"][0] =
            json::array({numQuestion, question, "", "Rédaction", "Question issue du site", ""});
        std::cout << "New question with : " << nouvelleQuestion.dump() << std::endl;
        // on spécifie les headers de la requete dont le jeton d'acces oauth2.
        const httplib::Headers headers = {{"Content-Type", "application/json"},
                                          {"Authorization", "Bearer " + mAccessToken}};
        // appel de la méthode Rest API avec le body contenant la nouvelle question.
        auto res = mHttpClient.Post("/v4/spreadsheets/" + mSpreadsheetId + "/values/" + mTab + "!" + mFields +
                                        ":append?valueInputOption=RAW&insertDataOption=INSERT_ROWS",
                                    headers, nouvelleQuestion.dump(), "application/json");
        std::cout << res->status << std::endl;
        std::cout << res->reason << std::endl;
        // on retourne si le status == 200 (ok) ou pas.
        return (res->status == 200);
    }
    /*
     * Pas d'update de question via le site pour  GoogleSpreadSheet on utilisera l'ihm de google.
     */
    virtual bool updateQuestion(int rowid, const std::string &reponse, bool reponse_valide)
    {
        return false;
    }
    /*
     * Pas de suppression via le site on utilisera l'ihm de google.
     */
    virtual bool deleteQuestion(int rowid)
    {
        return false;
    }

    virtual std::optional<std::vector<FAQRow>> getAllValidated()
    {
        // On récupère toutes les Q/R.
        auto allQR = getAll();
        if (allQR.has_value())
        {
            auto vQr = allQR.value();
            // On efface les Q/R qui n'ont pas la réponse valide.
            std::erase_if(vQr, [](const auto &row) { return !row.REPONSE_VALIDE; });
            return {vQr};
        }
        return std::nullopt;
    }
    virtual std::optional<std::vector<FAQRow>> getAll()
    {
        // Appel de l'API Google avec l'API_KEY fournie.

        auto res = mHttpClient.Get("/v4/spreadsheets/" + mSpreadsheetId + "/values:batchGet?ranges=" + mTab +
                                   "&key=" + mApiKey);
        std::cout << res->body << std::endl;
        if (!res->body.empty())
        {
            std::vector<FAQRow> retour;
            // récupération au format json du body de la réponse
            // contient l'ensemble des lignes du fichier excel.
            json sheet = json::parse(res->body);
            // extraction des lignes.
            auto lines = sheet["valueRanges"][0]["values"];
            if (!lines.empty())
            {
                // on efface l'entête qui contient l'intitulé des colonnes. (cad le 1er élément du vecteur)
                lines.erase(lines.begin());

                for (auto &line : lines)
                {
                    // pour chaque ligne on va créer un objet FAQRow et affecter les attributs à partir de la ligne
                    // json.
                    try
                    {
                        FAQRow row;
                        std::string rowidstr = line[0];
                        // Cette instruction peut échouer si la string fournie ne peut être convertie
                        // la ligne sera ignorée par le catch
                        row.ROWID = std::stoi(rowidstr);
                        row.QUESTION = line[1];
                        row.REPONSE = line[2];
                        row.REPONSE_VALIDE = line[3] == "Validé" ? true : false;
                        // on ajoute la ligne au retour.
                        retour.push_back(row);
                    } // on catch les éventuelles exceptions de conversion.
                    catch (const std::invalid_argument &e)
                    {
                        std::cout << "impossible d'instancier objet FAQRow : " << e.what() << " : " << line[0] << "\n";
                    }
                    catch (const std::out_of_range &e)
                    {
                        std::cout << "impossible d'instancier objet FAQRow : " << e.what() << " : " << line[0] << "\n";
                    }
                }
            }

            return {retour};
        }

        return std::nullopt;
    }

  private:
    /*
     * Méthode de mise à jour du jeton d'accès oauth2.
     */
    void majAccessToken()
    {
        // Si le jeton est invalide ou absent.
        if (Tools::currentTimestamp() >= mAccessTokenTimestamp || mAccessToken.empty())
        {
            // on génère un nouveau jeton JWT à partir de l'adresse email de l'utilisateur et de la clé privée.
            std::string token = Tools::JWTToken(mServiceAccount, "https://www.googleapis.com/auth/spreadsheets",
                                                "https://oauth2.googleapis.com/token", mPrivateKey);
            std::cout << "Token JWT : " << token << std::endl;
            // on appelle le point d'accès permettant de récupérer un jeton d'accès oauth2 a partir du jeton JWT.
            auto res =
                mHttpClient.Post("https://oauth2.googleapis.com/"
                                 "token?grant_type=urn%3Aietf%3Aparams%3Aoauth%3Agrant-type%3Ajwt-bearer&assertion=" +
                                 token);
            if (!res->body.empty())
            {
                // récupération du jeton d'accès dans le body de la réponse.
                mAccessToken = json::parse(res->body)["access_token"];
                // mise à jour du moment d'expiration (30 minutes dans le futur) du jeton.
                mAccessTokenTimestamp = Tools::currentTimestamp(std::chrono::minutes(30));
                // std::cout << "Access Token : " << mAccessToken << std::endl;
            }
        }
    }
    httplib::Client mHttpClient{"https://sheets.googleapis.com"};
    std::string mTab;
    std::string mFields;
    std::string mSpreadsheetId;
    std::string mApiKey;
    std::string mPrivateKey;
    std::string mServiceAccount;
    int64_t mAccessTokenTimestamp{0};
    std::string mAccessToken;
};
#endif
