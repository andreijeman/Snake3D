#include<stdio.h>
#include<stdlib.h>

typedef struct Node
{
    void *data;
    struct Node *next;
    struct Node *back;
}Node;

typedef struct List
{
    Node *head;
    Node *tail;
    int size;
}List;

List *create_list()
{
    List *list = malloc(sizeof(List));
    if(list)
    {
        list->head = NULL;
        list->tail = NULL;
        return list;
    }
    printf("Error: the list wasn't created!");
    return NULL;
}

Node *create_node(void *data)
{
    Node *node = malloc(sizeof(Node));
    if(node)
    {
        node->data = data;
        node->next = NULL;
        node->back = NULL;
        return node;
    }
    printf("Error: the node wasn't created!");
    return NULL;
}

void push_front(List *list, void *data)
{
    if(list)
    {
        Node *new_node = create_node(data);
        if(list->head)
        {
            list->head->back = new_node;
            new_node->next = list->head;
            list->head = new_node;
        }
        else
        {
            list->head = new_node;
            list->tail = new_node;
        }
    }
}

void push_back(List *list, void *data)
{
    Node *new_node = create_node(data);
    if(list)
    {
        if(list->head)
        {
            list->tail->next = new_node;
            new_node->back = list->tail;
            list->tail = new_node;
        }
        else
        {
            list->head = new_node;
            list->tail = new_node;
        }
    }
}

void pop_front(List *list)
{
    if(list)
    {
        Node *next = list->head->next;
        free(list->head);
        list->head = next;
    }
}

void clear_list(List *list)
{
    if(list)
    {
        Node *current_node = list->head, *back_node = NULL;
        while(current_node)
        {
            back_node = current_node;
            current_node = current_node->next;
            free(back_node);
        }
        list->head = NULL;
    }
}

void delete_list(List *list)
{
    clear_list(list);
    free(list);
}




