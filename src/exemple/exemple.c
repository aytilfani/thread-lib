#include <sys/queue.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[])
{
    SIMPLEQ_HEAD(filehead,  element) head;
    struct filehead *fhead;
    struct element
    {
        /* data */
        int value;
        SIMPLEQ_ENTRY(element) elements;
    } *n1, *n2, *n3, *np;
    
    SIMPLEQ_INIT(&head);
    n1 = malloc(sizeof(struct element));
    n1->value = 2;
    SIMPLEQ_INSERT_HEAD(&head, n1, elements);
    n2 = malloc(sizeof(struct element));
    n2->value = 3;
    SIMPLEQ_INSERT_HEAD(&head, n2, elements);
    n3 = malloc(sizeof(struct element));
    n3->value = 4;
    SIMPLEQ_INSERT_HEAD(&head, n3, elements);
    if (!SIMPLEQ_EMPTY(&head))
    {
        /* code */
        printf("File non vide \n");
    }
    SIMPLEQ_FOREACH(np, &head, elements) {
        printf("%d\n", np->value);
    }
    return 0;
}

