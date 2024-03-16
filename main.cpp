#include "GoogleSheetDataAccess.hpp"
#include "IDataAccess.hpp"
#include "SecurityManager.hpp"
#include "Tools.hpp"
#include <crow.h>
#include <crow/mustache.h>
using json = nlohmann::json;
/*
 * Méthode permettant de vérifier si l'utilisateur à l'origine de la requête est authentifié.
 */
bool userAuthenticated(const crow::request &request, SecurityManager &sm)
{
    // on récupère le cookie dans la requete.
    auto cookie = request.get_header_value("Cookie");
    if (!cookie.empty())
    {
        std::vector<std::string> tokenz;
        // on découpe le cookie en deux par le caractère =
        Tools::tokenizeString(cookie, std::regex("="), tokenz);
        // on vérifie la présence et la validité du token dans le SecurityManager.
        return sm.checkToken(tokenz[1]);
    }
    return false;
}
/*
 * Méthode de rendu conditionnel du template faq.
 */
crow::mustache::rendered_template populateTemplate(const std::string &route, const crow::request &request,
                                                   SecurityManager &sm, IDataAccess &dataAccess)
{
    crow::mustache::context ctx;

    std::optional<std::vector<FAQRow>> allQr;
    // route pour le back office (non utilisé avec google sheet utilisé seulement avec sqlite).
    if (route == "/back")
    {
        if (userAuthenticated(request, sm))
        {
            allQr = dataAccess.getAll();
            ctx["back"] = "true";
            ctx["login"] = "true";
        }
        else
        {
        }
    }
    else
    {
        // on considère l'utilisateur comme toujours authentifié pour le front.
        ctx["login"] = "true";
        // clé captchaClient pour génération d'un gToken via le widget recaptcha.
        ctx["captchaClient"] = sm.getCaptchaClient();
        // détermine si l'ip du client a le droit de poser une question, si oui on affiche le champ,
        // si non on le masque.
        if (sm.showAskQuestion(request.remote_ip_address))
            ctx["askQuestion"] = "true";

        // récupération de toutes les Q/R validées.
        allQr = dataAccess.getAllValidated();
    }

    if (allQr.has_value())
    {
        // Conversion puis affectation au template des Q/R retournées par la couche de données.
        ctx["allQr"] = Tools::convertListToWValue(allQr.value());
        // Sers à alimenter un champ caché de comptage des réponses affichées.
        ctx["numQuestion"] = allQr.value().size();
    }
    // on charge le template et on effectue le rendu html avec le contexte fourni.
    auto page = crow::mustache::load("faq.mustache.html");
    return page.render(ctx);
}
/*
 * Méthode principale.
 * argv[1] doit contenir le nom d'un fichier json valide de configuration
 * {
  "captchaClient":"CLIENT_KEY",
  "captchaSecret":"SECRET_KEY",
  "visitorsCanAskQuestions":true,
  "ipProtection":true,
  "visitorsAskingDelay":1440,
  "spreadsheetId":"SPREADSHEET_ID",
  "apikey":"API_KEY",
  "tab":"Sheet1",
  "fields":"A1:G1",
  "serviceAccount":"xxx.Xxx@iam.gserviceaccount.com",
  "privateKey":"--- private key ---"
}
*/
int main(int argc, char *argv[])
{
    // vérification du nombre d'arguments.
    if (argc >= 2)
    {
        crow::SimpleApp app;

        // lecture et parsing du fichier de configuration.
        std::ifstream f(argv[1]);
        json data = json::parse(f);

        // instanciation du SecurityManager avec les différents paramètres du fichier de configuration.
        // l'ipProtection doit être désactivée si le serveur se trouve derrière un proxy (Nginx, apache2...)
        SecurityManager sm(data["captchaClient"], data["captchaSecret"], "oiedmin", "poissword",
                           data["visitorsAskingDelay"], data["visitorsCanAskQuestions"], data["ipProtection"]);

        //    SqliteDataAccess da = SqliteDataAccess("faq.db");
        //    Instanciation du IDataAccess GoogleSheetDataAccess avec les paramètres du fichier de configuration fourni.
        GoogleSheetDataAccess gda = GoogleSheetDataAccess(data["spreadsheetId"], data["apikey"], data["tab"],
                                                          data["privateKey"], data["serviceAccount"], data["fields"]);

        // Affectation du GoogleSheetDataAccess dans l'interface qui sera utilisée dans la suite du programme.
        IDataAccess &dataAccess = gda;

        // Défintion des routes HTTP.
        //
        //  Route principale de la faq, sert à afficher la liste des questions/réponses et éventuellement le formulaire
        //  de saisie d'une question.
        CROW_ROUTE(app, "/faq")
        ([&sm, &dataAccess](const crow::request &request) {
            return populateTemplate("/faq", request, sm, dataAccess);
        });
        // Route permettant d'ajouter une question dans le stockage.
        CROW_ROUTE(app, "/question").methods(crow::HTTPMethod::Post)([&dataAccess, &sm](const crow::request &req) {
            std::string retour = "Erreur.";
            // On vérifie que l'IP source a le droit d'ajouter une question (normalement le formulaire est masqué mais
            // la route est toujours disponible coté serveur et doit donc être protégée).
            if (sm.checkIp(req.remote_ip_address))
            {
                // récupération des paramètres du formulaire à partir du body de la requête.
                auto bodyParams = req.get_body_params();
                // validation du jeton recaptcha
                if (sm.validateCaptcha(bodyParams.get("g-recaptcha-response")))
                {
                    // création de la question dans le stockage de données.
                    unsigned int numQuestion = Tools::extractInteger(bodyParams, "numQuestion");
                    // récupération de la question dans le corps de la requête et vérification de sa longueur.
                    std::string question = bodyParams.get("input-question");
                    if (question.length() <= 200)
                    {
                        if (dataAccess.createQuestion(question, numQuestion + 1))
                        {
                            // On enregistre l'IP dans le SecurityManager pour interdire de poser une nouvelle question
                            // pendant 24h
                            sm.registerIp(req.remote_ip_address);
                            // Message de retour OK.
                            retour = "Merci d'avoir posé votre question.";
                        }
                        else
                        {
                            retour = "Erreur lors de l'enregistrement de la question.";
                        }
                    }
                    else
                    {
                        retour = "Votre question est trop longue (200 caractères max) veuillez la reformuler";
                    }
                }
                else
                {
                    retour = "Recaptcha invalide, veuillez reéssayer en veillant à bien cocher la case 'Je ne suis pas "
                             "un robot'";
                }
            }
            else
            {
                retour = "Vous avez déjà posé votre question aujourd'hui, veuillez réessayer plus tard.";
            }
            return retour;
        });
        // les routes du back office ont été désactivées car on utilise désormais google doc à la place de sqlite.
        //
        //
        //
        //
        //
        //
        // =========================================================================================================
        /*   CROW_ROUTE(app, "/login").methods(crow::HTTPMethod::Post)([&sm](const crow::request &req) {
               auto bodyParams = req.get_body_params();
               auto authentication = sm.authenticate(bodyParams.get("login"), bodyParams.get("password"));

               crow::response reponse;

               if (authentication.has_value())
               {
                   reponse.set_header("Set-Cookie", "session_id=" + authentication.value() +
                                                        "; HttpOnly; Secure; SameSite=Lax; Max-Age=1800;");
                   reponse.set_header("HX-Redirect", "/back");
               }
               else
               {
                   reponse.code = 403;
               }
               return reponse;
           });*/
        // define your endpoint at the root directory

        /*CROW_ROUTE(app, "/back")
            .methods(crow::HTTPMethod::Get, crow::HTTPMethod::Post)([&sm, &dataAccess](const crow::request &request) {
                return populateTemplate("/back", request, sm, dataAccess);
            });

        CROW_ROUTE(app, "/question/<int>")
            .methods(crow::HTTPMethod::Delete)([&sm, &dataAccess](const crow::request &req, int rowid) {
                if (userAuthenticated(req, sm))
                {
                    if (dataAccess.deleteQuestion(rowid))
                    {
                        return crow::response(200);
                    }
                }
                return crow::response(400);
            });*/

        /*CROW_ROUTE(app, "/question/<int>")
            .methods(crow::HTTPMethod::Put)([&sm, &dataAccess](const crow::request &req, int rowid) {
                if (userAuthenticated(req, sm))
                {
                    auto bodyParams = req.get_body_params();
                    auto keys = bodyParams.keys();

                    if (dataAccess.updateQuestion(
                            rowid, bodyParams.get("reponse-" + std::to_string(rowid)),
                            std::count(keys.begin(), keys.end(), "reponse_valide-" + std::to_string(rowid)) > 0 ? true
                                                                                                                :
           false))
                    {
                        return crow::response(200);
                    }
                }
                return crow::response(400);
            });*/
        // ==============================================================================================================================
        //
        //
        //
        // Démarrage du serveur http.
        app.port(18080).multithreaded().run();
    }
    else
    {
        std::cout << "Nombre d'arguments insuffisant" << std::endl;
    }
}
