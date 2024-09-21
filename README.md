Ce projet a été réalisé par 3 étudiants à l'UQAC :  
Romain CHARRONDIERE  
Maxime PAINCHAUD  
Emile VEILLETTE  

L'objectif du programme est de mettre un relation des serveurs avec des clients afin d'y enregistrer des ressources.  
Le client peut utiliser différents protocoles (rdo ou wrdo) pour ajouter une ressource.  
Chaque ressource peut également en référencer d'autres via un format spécifique ( [Protocol]://[IP_Server]:[Port]/[Resource_Id] ).  
Lorsqu'un client demande l'accès à une ressource, le serveur renvoie alors les données de cette dernière et toutes celles qu'elle référence.  
De plus, la particularité du protocole wrdo est qu'il permet à l'utilisateur de créer un abonnement à une ressources. Ainsi, le client sera notifié à chaque fois que la ressource est modifiée par un autre utilisateur.

Le projet est déceloppé en C++.  
La librairie nlohmann a été utilisée pour la lecture des fichiers json.  
Des ressources exemples sont disponibles dans le dossier "ressources" côté client.
