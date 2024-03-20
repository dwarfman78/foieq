# FOIEQ - Serveur de gestion d'une FAQ dynamique !

Ce serveur sert une page de FAQ (Frequently Asked Question) dynamique basée sur une spreadsheet Google Docs.

# Dépendances system

-   libstd++
-   build-base
-   cmake
-   openssl-dev
-   asio-dev

# Dépendances incluses

-   Crow
-   cpp-httplib
-   nholman/json
-   jwt-cpp
-   picojson
-   SQLiteCpp

# Compilation

Placez vous dans le dossier `lib/Crow/build` et lancez la commande suivante :  
`cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF`  
puis  
`make install`

Placez vous dans le dossier racine et lancez la commande suivante :  
`cmake .`  
puis  
`make`

# Configuration

Exemple de configuration (config.json):

```
{
  "captchaClient":"CLIENT_KEY",
  "captchaSecret":"SECRET_KEY",
  "visitorsCanAskQuestions":true,
  "ipProtection":true,
  "visitorsAskingDelay":1440,
  "spreadsheetId":"SPREADSHEET_ID",
  "apikey":"API_KEY",
  "tab":"Sheet1",
  "fields":"A1:G1",
  "serviceAccount":"SERVICE_ACCOUNT",
  "privateKey":"PRIVATE_KEY"
}
```

**captchaClient** : la clé publique Google reCAPTCHA.  
**captchaSecret** : la clé privée Google reCAPTCHA.  
**visitorsCanAskQuestions** : Active ou désactive le formulaire de saisie de questions.  
**ipProtection** : limite le nombre d'appel au backend par adresse IP (à désactiver si backend est derrière un proxy NGINX/Apache2)  
**visitorsAskingDelay** : temps mini entre deux ajout de questions par la même IP.  
**spreadsheetId** : identifiant de la feuille qui servira de stockage aux questions.  
**apikey** : API_KEY de Google Cloud API pour permettre de LIRE la feuille.  
**tab** : le nom de la tab dans la feuille.  
**fields** : les champs d'ajout de question.  
**serviceAccount** : l'adresse Google pour le service account qui servira à AJOUTER des questions.  
**privateKey** : la clé privée RSA générée dans la console Google Cloud qui permet de signer le jeton JWT  
pour récupérer la clé OAUTH2 de modification de la feuille.

# Docker

Assurez-vous que le dossier `lib/Crow/build` est vide puis lancez la commande :  
`docker build -t NOM_DU_TAG_SOUHAITE .`

Lancement :  
`docker run -v /path/to/config.json:/foieq/config.json -p 18080:18080 NOM_DU_TAG_SOUHAITE`

# URL

`http://localhost:18080/faq`

