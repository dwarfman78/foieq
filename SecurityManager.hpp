#ifndef FAQ_SECURITYMANAGER_HPP
#define FAQ_SECURITYMANAGER_HPP
#include "Tools.hpp"
#include "json/json.hpp"
#include <map>
#include <optional>
#include <string>
using json = nlohmann::json;
/*
 * Classe permettant de gérer la sécurité.
 */
class SecurityManager
{
  public:
    /*
     * Constructeur
     * @param pCaptchaClient : clé pour le client captcha.
     * @param pCaptchaSecret : clé privée captcha.
     * @param pLogin : identifiant de l'administrateur.
     * @param pPassword : mot de passe de l'administrateur.
     * @param pIpNextTryTime : temps en minutes pendant lequel une même ip ne peut plus poser de question.
     * @param pShowAskQuestion : permission d'afficher ou non l'ajout de question.
     * @param pIpProtection : activation de la protection par Ip.
     */
    SecurityManager(const std::string &pCaptchaClient, const std::string &pCaptchaSecret,
                    const std::string &pLogin = "oiedmin", const std::string &pPassword = "poiessword",
                    unsigned int pIpNextTryTime = 1440, bool pShowAskQuestion = false, bool pIpProtection = true)
        : mCaptchaClient(pCaptchaClient), mCaptchaSecret(pCaptchaSecret), mLogin(pLogin), mPassword(pPassword),
          mIpNextTryTime(pIpNextTryTime), mShowAskQuestion(pShowAskQuestion), mIpProtection(pIpProtection)
    {
    }
    bool validateCaptcha(const std::string &gToken)
    {
        // https://www.google.com/recaptcha/api/siteverify
        // secret
        // response
        auto res = mHttpClient.Post("/recaptcha/api/siteverify?secret=" + mCaptchaSecret + "&response=" + gToken);

        if (res->status == 200)
        {
            json bodyResponse = json::parse(res->body);

            return bodyResponse["success"];
        }
        return false;
    }
    /*
     * Méthode d'authentification de l'utilisateur
     * @param login : identifiant.
     * @param password : mot de passe.
     * @return le jeton d'authentification de l'utilisateur.
     */
    std::optional<std::string> authenticate(const std::string &login, const std::string &password)
    {
        // Si l'identifiant et le mot de passe en paramètre correspondent à ceux fourni au constructeur.
        if (login == mLogin && password == mPassword)
        {
            // on génère un token et un timestamp 30 minutes dans le futur.
            std::string token = Tools::uuidFromTimestamp();
            int64_t timestamp = Tools::currentTimestamp(std::chrono::minutes(30));
            // on enregistre le timestamp sous la clé du token unique.
            mTokens[token] = timestamp;
            // on retourne le token pour l'envoyer à l'utilisateur.
            return {token};
        }
        else
        {
            // sinon on retourne null;
            return std::nullopt;
        }
    }
    /*
     * Méthode de vérification de la validité d'un token, les tokens sont valables 30 minutes.
     * @param token : le token à vérifier.
     * @return vrai si le token est valide et faux sinon.
     */
    bool checkToken(const std::string &token)
    {
        // timestamp actuel.
        auto now = Tools::currentTimestamp();
        // on efface tous les tokens expirés (ceux qui sont dans le passé par rapport à maintenant)
        std::erase_if(mTokens, [&now](const auto &item) { return std::get<1>(item) < now; });
        // une fois tous les tokens expirés effacés on recherche notre token à tester.
        auto mToken = mTokens.find(token);
        // si le token est trouvé c'est qu'il est valide, sinon il est invalide.
        return mToken != mTokens.end();
    }
    /*
     * Méthode d'enregistrement de l'adresse IP, l'adresse en tant que telle n'est pas stockée mais
     * un hash unique est calculé pour cela.
     * @param ip : l'adresse ip à enregistrer dans le system.
     */
    void registerIp(const std::string &ip)
    {
        if (mIpProtection)
        {
            // calcul du timestamp dans le futur (24h par défaut) et du hash de l'ip.
            auto nextTry = Tools::currentTimestamp(std::chrono::minutes(mIpNextTryTime));
            auto ipHash = Tools::sha256(ip);
            // on enregistre le timestamp sous la clé du hash de l'ip
            mIpNextTry[ipHash] = nextTry;
        }
    }
    /*
     * Méthode de vérification de l'ip.
     * @param ip : IP à vérifier.
     * @return vrai si l'ip est valide et peut accéder au service, faux si l'ip a déjà tenté
     * d'accéder au service dans le temps déterminé (24h par défaut).
     */
    bool checkIp(const std::string &ip)
    {
        // on recalcul le hash de l'ip.
        auto ipHash = Tools::sha256(ip);
        // on cherche le timestamp limite à partir duquel l'ip pourra de nouveau appeler le service.
        auto ipFound = mIpNextTry.find(ipHash);
        // si l'ip est absente ou si elle est présente et la limite est dépassée alors l'ip peut
        // appeler le service.
        return !mIpProtection ||
               (mIpProtection && (ipFound == mIpNextTry.end() ||
                                  (ipFound != mIpNextTry.end() && Tools::currentTimestamp() >= ipFound->second)));
    }
    /*
     * Méthode permettant de déterminer si l'utilisateur peut ou non saisir une question.
     * @param ip : l'adresse ip de l'utilisateur.
     * @return vrai s'il peut saisir une question, faux sinon. Cela est déterminé par deux critères :
     * - la validité de son adresse IP cad s'il a déjà saisi une question dans les 24h (par défaut)
     * - un booléen maitre issu du constructeur de la classe.
     */
    bool showAskQuestion(const std::string &ip)
    {
        return mShowAskQuestion && checkIp(ip);
    }

    const std::string &getCaptchaClient() const
    {
        return mCaptchaClient;
    }

  private:
    // client http pour vérification captcha
    httplib::Client mHttpClient{"https://www.google.com"};
    // identifiant d'admin
    const std::string mLogin;
    // mot de passe admin.
    const std::string mPassword;
    // map des jetons / timestamp de validité du jeton.
    std::map<std::string, int64_t> mTokens;
    // map des ip / timestamp de blocage de l'ip.
    std::map<std::string, int64_t> mIpNextTry;
    // temps de blocage de l'ip en minutes. (1440 par défaut soit 24h)
    unsigned int mIpNextTryTime;
    // master switch pour masquer l'autorisation de poser une question.
    bool mShowAskQuestion{false};
    // master switch pour la protection IP.
    bool mIpProtection{true};
    // clé client recaptcha
    const std::string mCaptchaClient;
    // clé privée recaptcha
    const std::string mCaptchaSecret;
};
#endif
