#ifndef FAQ_IDATAACCESS_HPP
#define FAQ_IDATAACCESS_HPP
#include "FAQRow.hpp"
#include <optional>
#include <vector>
/*
 * Interface d'accès aux données.
 */
class IDataAccess
{
  public:
    /*
     * Méthode de création d'une nouvelle question sans réponse.
     * @param question : contenu de la question.
     * @param numQuestion : numéro de la question (optionel).
     * @return vrai si question ajoutée, faux sinon.
     */
    virtual bool createQuestion(const std::string &question, unsigned int numQuestion = 0) = 0;
    /*
     * Méthode de mise à jour d'une question/réponse existante.
     * @param rowid : identifiant unique de la question.
     * @param reponse : contenu de la réponse.
     * @param reponse_valide : réponse valide oui/non.
     * @return vrai si la question a été mise à jour faux sinon.
     */
    virtual bool updateQuestion(int rowid, const std::string &reponse, bool reponse_valide) = 0;
    /*
     * Méthode de suppression d'une question.
     * @param rowid : identifiant unique de la question à supprimer.
     * @return vrai si question supprimée faux sinon.
     */
    virtual bool deleteQuestion(int rowid) = 0;
    /* Méthode permettant de récuperer toutes les question/réponses avec le statut validé.
     * @return optional avec les question/réponses avec le statut validé ou null.
     */
    virtual std::optional<std::vector<FAQRow>> getAllValidated() = 0;
    /*
     * Méthode permettant de récupérer toutes les question/réponses.
     * @return optional avec les questions/réponses ou null;
     */
    virtual std::optional<std::vector<FAQRow>> getAll() = 0;
    virtual ~IDataAccess()
    {
    }
};
#endif
