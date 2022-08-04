# Threads en espace utilisateur

## Utilisation

Pour compiler le projet :
```
$ make
```

Pour lancer les tests:
```
$ make check
```

Pour lancer valgrind:
```
$ make valgrind
```

Pour compiler l'ensemble de tests avec la bibliothèque pthread
```
$ make pthreads
```

Pour activer la préemption :

Décommenter la ligne 12 du fichier thread.c : #define USE_PREEMPTION
afin d'activer la préemption avec les macros.

## Auteurs

DUCOS Mathieu
DUJARDIN Louis
LEBARBIER Yves
PAUWELS Quentin
TILFANI Aymen