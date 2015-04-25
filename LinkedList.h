typedef struct data{
  char *date;
  char *day;
  int high;
  int low;
  char *description;
} Data;

typedef struct node{
  Data data;
  struct node *next;
} Node, *NodePtr;

typedef struct linkedlist{
  NodePtr head;
} LinkedList, *LinkedListPtr;

LinkedListPtr initLinkedList();
/* allocates dynamic memory for the linked list, initializes its head to NULL, returns a pointer to it */

Data check(LinkedListPtr list, int position); /*returns the data in the linked list at the given position; position = 0 corresponds to the first node of linked list. */
void replace(LinkedListPtr list, int position, Data data);  /* replaces the data in the linked list at the given position.*/
void display(LinkedListPtr list); /*displays all the data in the linked list */
void displayTo(LinkedListPtr list, int numElements); /* display x elements */
void add(LinkedListPtr list, Data data, int position); /* inserts the data in the linked list at the given position, for position = 0  insert before the head,  for position >= number of nodes  insert after the tail */
void remove_(LinkedListPtr list, int position); /*removes the data in the linked list at the given position, for position = 0  remove the head,  for position = number of nodes – 1 remove the tail node, for position >=number of nodes don’t remove anything*/
void empty(LinkedListPtr list); /*removes every node from the linked list */
int isEmpty(LinkedListPtr list); /*checks if a linked list is empty (head ==NULL), returns 1 if yes, 0 otherwise */
LinkedListPtr destroy(LinkedListPtr list); /* checks if a linked list is empty, if it is - frees the dynamic memory allocated for the empty linked list, returns a NULL pointer to it */
