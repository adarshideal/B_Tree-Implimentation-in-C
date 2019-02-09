#include<stdio.h>
#include <stdlib.h>
#include <ctype.h>
#define MAX 4

struct data {
	char word[20];
	char meaning[100];
};

struct meta_data {
	int node_count;
	int root_pos;
};


struct node {
        struct data keys[MAX - 1];  // An array of keys
        int child_ptr[MAX]; // An array of child pointers
        int n;     // Current number of keys
        int leaf; // Is true when node is leaf. Otherwise false
        int pos;// to store the position of the node
};

struct meta_data *init_tree(int flag)
{
	FILE *fp;
	struct meta_data *tree = NULL;
	tree = (struct meta_data *) malloc(sizeof(struct meta_data));
	if (flag == 0)
	{
		tree->node_count = 1;
		tree->root_pos = 0;

		fp = fopen("dictionary.bin", "r+");
		fclose(fp);

		return tree;
	}
	fp = fopen("dictionary.bin", "r+");
	fseek(fp, 0, 0);
	fread(tree, sizeof(tree), 1, fp);

	return tree;
}

void file_write(struct node *node, int pos)
{
	FILE *fp = fopen("dictionary.bin", "r+");
	fseek(fp, sizeof(struct node) * pos, 0);
	fwrite(node, sizeof(struct node), 1, fp);
	fclose(fp);
}

void file_read(struct node *node, int pos)
{
	FILE *fp = fopen("dictionary.bin", "r+");
	fseek(fp, sizeof(struct node) * pos, 0);
	fread(node, sizeof(struct node), 1, fp);
	fclose(fp);
}

void write_meta(struct meta_data *tree)
{
	FILE  *fp;
	fp = fopen("dictionary.bin", "r+");
	fseek(fp, 0, 0);
	fwrite(tree, sizeof(tree), 1, fp);
	fclose(fp);
}

struct node *create_node(struct meta_data *tree, int leaf)
{
	int i;
	struct node *new_node = (struct node *) malloc(sizeof(struct node));
	new_node->leaf = leaf;
	new_node->pos = tree->node_count;
	tree->node_count++;
	new_node->n = 0;

	for(i = 0; i < MAX; i++) {
		new_node->child_ptr[i] = -1;
	}

	return new_node;
}

void split_node(struct meta_data *tree, struct node *parent, int i, struct node *left_child)
{
	struct node *right_child = create_node(tree, left_child->leaf);
	int t;
	t = MAX / 2;
	right_child->n = t - 1;

	int j;
	for (j = 0; j < t - 1; j++) {
		right_child->keys[j] = left_child->keys[j + t];
	}
	if (!left_child->leaf) {
		for (j = 0; j < t; j++) {
			right_child->child_ptr[j] = left_child->child_ptr[j + t];
		}
	}

	left_child->n = t - 1;
	for (j = parent->n; j >= i + 1; j--) {
		parent->child_ptr[j + 1] = parent->child_ptr[j];
	}

	parent->child_ptr[i + 1] = right_child->pos;

	for (j = parent->n - 1; j >= i; j--) {
		parent->keys[j + 1] = parent->keys[j];
	}

	parent->keys[i] = left_child->keys[t - 1];

	parent->n++;

	file_write(parent, parent->pos);
	file_write(left_child, left_child->pos);
	file_write(right_child, right_child->pos);
	free(right_child);

}

void insert_non_full(struct meta_data *tree, struct node *node, struct data *word_data)
{
	int i;

	i = node->n - 1;

	if(node->leaf == 1) {
		while(i >= 0 && strcmp(node->keys[i].word, word_data->word) > 0) {
			node->keys[i + 1] = node->keys[i];
			i--;
		}
		node->keys[i + 1] = *word_data;
		(node->n)++;
		file_write(node, node->pos);
	} else {
		while(i >= 0 && strcmp(node->keys[i].word, word_data->word) > 0)
			i--;

		struct node *temp = malloc(sizeof(struct node));

		file_read(temp, node->child_ptr[i + 1]);

		if(temp->n == MAX - 1)
		{
			split_node(tree, node, i + 1, temp);//split child node

			if(strcmp(node->keys[0].word, word_data->word) < 0)
				i++;
		}
		file_read(temp, node->child_ptr[i + 1]);

		insert_non_full(tree, temp, word_data);

		free(temp);
	}
}
void insert(struct meta_data *tree, struct data *word_data)
{
	if(tree->node_count == 1) {
		struct node *new_node = create_node(tree, 1);
		new_node->keys[0] = *word_data;
		(new_node->n)++;
		tree->root_pos = new_node->pos;
		file_write(new_node, new_node->pos);
		free(new_node);
		return;
	} else {
		struct node *root = malloc(sizeof(struct node));
		file_read(root, tree->root_pos);
		if(root->n == MAX - 1)
		{
			struct node *new_root = create_node(tree, 0);
			new_root->child_ptr[0] = tree->root_pos;

			split_node(tree, new_root, 0, root);
			struct node *node = (struct node *) malloc(sizeof(struct node));
			file_read(node, root->pos);

			int i = 0;
			if(strcmp(new_root->keys[0].word, word_data->word) < 0)
				i++;

			insert_non_full(tree, node, word_data);
			tree->root_pos = new_root->pos;

			file_write(node, node->pos);
			free(new_root);
			free(node);
		} else {
			insert_non_full(tree, root, word_data);
		}
		free(root);
	}

}

struct data *search_data(struct meta_data *tree, struct node *root, char *word)
{
	int i = 0;

	while(i < root->n && strcmp(root->keys[i].word, word) < 0) {
		printf("%s\n", root->keys[i].word);
		i++;
	}

	if(i < root->n && (strcmp(root->keys[i].word, word) == 0)) {
		return &root->keys[i];
	} else if(root->leaf == 1) {
		return NULL;
	} else {
		struct node *next_node = malloc(sizeof(struct node));
		file_read(next_node, root->child_ptr[i]);

		return search_data(tree, next_node, word);
	}
}

struct data *search(struct meta_data *tree, char *word)
{
	struct node *root = malloc(sizeof(struct node));
	file_read(root, tree->root_pos);

	return search_data(tree, root, word);
}

int main()
{
	struct data *word_data = NULL;
	struct meta_data *tree = NULL;
	char word[20];
	int ch;

	tree = init_tree(0);

	//printf("trre-> %lu", sizeof(word_data);
	printf("\n----------------Welcome to Dictinary------------------\n");
	printf("\n%d %d\n", tree->node_count, tree->root_pos);
	//int size_node = sizeof(struct data);
	//printf("%d is size of node\n", size_node);
	//printf("%d is 2t - 1 or m - 1 \n", 1940/size_node);
	// //printf("trre-> %lu", sizeof(word_data));
/*	int z = 1315;
	while(z--) {
	 	word_data = (struct data *) malloc(sizeof(struct data));
	 	scanf("%s", word_data->word);
	 	scanf("%[^\n]%*c", word_data->meaning);
	 	//printf("word : %s meaning : %s\n", word_data->word, word_data->meaning);
	 	insert(tree, word_data);
	}
*/	while(1) {
		printf("\n1. Insert\n2. Search\n3. Exit\nEnter your choice: ");
		scanf("%d",&ch);
		word_data = (struct data *) malloc(sizeof(struct data));
		switch(ch) {
			case 1:
				printf("\nEnter word: ");
				scanf(" %[^\n]%*c", word_data->word);
				printf("\nEnter meaning: ");
				scanf(" %[^\n]%*c", word_data->meaning);
				insert(tree, word_data);
				/*int i;
				struct node *new_node = malloc(sizeof(struct node));
				file_read(new_node, tree->root_pos);
				printf("\nn = %d\n", new_node->n);
				for (i = 0; i < new_node->n; i++) {
					printf("%s", new_node->keys[i].word);
				}*/
				break;
			case 2:
				printf("\nEnter word to search: ");
				scanf("%s", word);
				word_data = NULL;
				word_data = search(tree, word);
				if (word_data == NULL) {
					printf("\nWord not Found!!");
				} else {
					printf("Word : %s\n", word);
					printf("Meaning : %s", word_data->meaning);
				}
				break;
			case 3:
				write_meta(tree);
				exit(1);
			default:
				printf("Wrong Choice Stupid!!");
		}
	}

	write_meta(tree);
//	printf("\n%d %d\n", tree->node_count, tree->root_pos);
//	printf("success\n");

	return 0;
}
