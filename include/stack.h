// vim: noet ts=4 sw=4
#pragma once
/* This stack shamelessly stolen from OlegDB. */
typedef struct ol_stack {
	const void *data;
	struct ol_stack *next;
} ol_stack;

const void *spop(ol_stack **stack);
void spush(ol_stack **stack, const void *data);
