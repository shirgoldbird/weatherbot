#include <stdio.h>
#include <stdlib.h>
#include "LinkedList.h"

//allocates dynamic memory for the linked list, initializes its head to NULL, returns a pointer to it
LinkedListPtr initLinkedList(){
	LinkedListPtr list;
	if ((list = (LinkedListPtr)malloc(sizeof(LinkedList))) == NULL){
		fprintf(stderr, "Out of memory\n");
    exit(1);
  }
	list->head = NULL;
	return list;
}

Data check(LinkedListPtr list, int position){
	int i;
	NodePtr node = list->head;
	for(i = 0; i < position; i++){
		node = node->next;
	}
	return node->data;
} /*returns the data in the linked list at the given position; position = 0 corresponds to the first node of linked list. */

void replace(LinkedListPtr list, int position, Data data){
	int i;
	NodePtr node = list->head;
	for(i = 0; i < position; i++){
		node = node->next;
	}
	node->data = data;
}  /* replaces the data in the linked list at the given position.*/

/*displays all the data in the linked list */
void display(LinkedListPtr list){
	NodePtr node = list->head;
	printf("Linked List: ");

	if (list->head == NULL)
		printf("Empty");

	while (node!=NULL){
		printf("%s\n", node->data.description);
		node=node->next;
	}
	printf("\n");
}

void displayTo(LinkedListPtr list, int numElements){
	NodePtr node = list->head;
	int i = 0;
	printf("Elements 1-%d of Linked List: ", numElements);
	if(list->head == NULL){
		printf("Empty!");
	}
	else{
		while(node != NULL && i < numElements){
			printf("%s\n", node->data.description);
			node = node->next;
			i++;
		}
	}
	printf("\n");
}

void add(LinkedListPtr list, Data data, int position){
	NodePtr newNode, nodeBefore, nodeAfter;
	int i;
	newNode = (NodePtr)malloc(sizeof(Node));
	if (newNode == NULL){
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	newNode->data = data;
	newNode->next = NULL;
	if(position == 0){
		if(list->head==NULL){
			list->head = newNode;
		}
		else{
			newNode->next = list->head;
			list->head = newNode;
		}
	}
	else{
		nodeAfter = list->head;
		for(i = 0; i < position; i++){
			if(nodeAfter != NULL){
				nodeBefore = nodeAfter;
				nodeAfter = nodeBefore->next;
			}
			else{
				nodeBefore->next = newNode;
				break;
			}
		}
		nodeBefore->next = newNode;
		newNode->next = nodeAfter;
	}
} /* inserts the data in the linked list at the given position, for position = 0 insert before the head, for position >= number of nodes insert after the tail */

void remove_(LinkedListPtr list, int position){
	int i;
	NodePtr node, nodeBefore, nodeAfter;
	if (list->head!=NULL){  /*linked list is not empty*/
		node = list->head;
		nodeAfter = node->next;
		if(position == 0){
				list->head=list->head->next;
				free(node);
			}
		else{
			for(i = 0; i < position && node->next != NULL; i++){
				nodeBefore = node;
				node = nodeAfter;
				nodeAfter = node->next;
			}
			nodeBefore->next = nodeAfter;
			free(node);
		}
	}
} /*removes the data in the linked list at the given position, for position = 0 remove the head, for position = number of nodes – 1 remove the tail node, for position
>=number of nodes don’t remove anything*/

void empty(LinkedListPtr list){
	NodePtr node, nodeAfter;
	node = list->head;
	while(node != NULL && node->next != NULL){
		nodeAfter = node->next;
		free(node);
		node = nodeAfter;
		nodeAfter = node->next;
	}
	list->head = NULL;
} /*removes every node from the linked list */

int isEmpty(LinkedListPtr list){
	if(list->head == NULL){
		return 1;
	}
	return 0;
} /*checks if a linked list is empty (head == NULL), returns 1 if yes, 0 otherwise */

LinkedListPtr destroy(LinkedListPtr list){
	if(isEmpty(list) == 1){
		free(list);
		list = NULL;
	}
	return list;
} /* checks if a linked list is empty, if it is - frees the dynamic memory allocated for the empty linked list, returns a NULL pointer to it */
