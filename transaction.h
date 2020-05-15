// Header guards
#ifndef TRANSACTION_H
#define TRANSACTION_H

#ifndef MATH_H
#define MATH_H
#include <math.h>
#endif

#ifndef STDLIB_H
#define STDLIB_H
#include <stdlib.h>
#endif

#ifndef STDBOOL_H
#define STDBOOL_H
#include <stdbool.h>
#endif

#include "coin.h"
#include "list.h"
#include "keygen.h"
#include "hash.h"
#include "bytes.h"
#include "ints.h"
#include "wallet.h"
#include "transactionio.h"

// Estrutura de uma transação
typedef struct _transaction {
    buffer id;
    rsaKey senderKey;
    unsigned long recipientKey;
    float value;
    ibuffer signature;

    list *inputs;
    list *outputs;
} transaction;

// Obter a hash da transação
buffer get_transaction_hash(transaction t) {
    buffer input = new_buffer(sizeof(long) + sizeof(float) + sizeof(rsaKey));

    int i;
    for(i = 0; i < sizeof(long); i++){
        input.bytes[i] = t.recipientKey & (0xff << (i * 8));
    }
    int v = t.value * pow(10, DECIMAL_PLACES);
    for(i; i < sizeof(long) + sizeof(float); i++){
        input.bytes[i] = v & (0xff << (i * 8));
    }
    for(i; i < sizeof(long) + sizeof(float) + sizeof(long); i++){
        input.bytes[i] = t.senderKey.key & (0xff << (i * 8));
    }
    for(i; i < sizeof(long) + sizeof(float) + sizeof(rsaKey); i++){
        input.bytes[i] = t.senderKey.n & (0xff << (i * 8));
    }

    buffer output = hash(input);
    free(input.bytes);
    return output;
}

// Checar assinatura
bool check_signature(transaction t){
    buffer claimedHash = decrypt(t.signature, t.senderKey);
    buffer originalHash = get_transaction_hash(t);
    return compare_buffer(claimedHash, originalHash);
}

// Gerar nova transação
transaction *new_transaction(rsaKey senderKey, long unsigned recipientKey, float value, list *inputs){
    transaction *output = malloc(sizeof(transaction));
    output->senderKey = senderKey;
    output->recipientKey = recipientKey;
    output->value = value;
    output->inputs = inputs ? inputs : new_list();
    output->outputs = new_list();

    buffer transactionHash = get_transaction_hash(*output);
    output->id = transactionHash;
    return output;
}

static float get_transaction_inputs_value(transaction t) {
    float total = 0;
    hashnode* temp = t.inputs->first;
    while(temp) {
        if(((transactionin*)temp->val)->output) {
           total += ((transactionin*)temp->val)->output->value;
        }        
        temp = temp->next;
    }
    return total;
}

static float get_transaction_outputs_value(transaction t) {
    float total = 0;
    hashnode* temp = t.outputs->first;
    while(temp) {
        total += ((transactionout*)temp->val)->value;
        temp = temp->next;
    }
    return total;
}

bool process_transaction(transaction *t, hashmap *outputs) {
    if(check_signature(*t) == false) {
        printf("\nTransaction signature does not match.");
        return false;
    }
    listnode *temp = t->inputs->first;
    while(temp) {
        ((transactionin*)(temp->val))->output = get_hashmap_val(outputs, ((transactionin*)(temp->val))->outputId);
        temp = temp->next;
    }
    if(t->value < MIN_TRANSACTION) {
        printf("\nTransaction value too small.");
        return false;
    }
    float inputs = get_transaction_inputs_value(*t);
    if(inputs < t->value) {
        printf("\nInsufficient funds.");
        return false;
    }
    float remainder = inputs - t->value;
    t->id = get_transaction_hash(*t);

    transactionout *senderToRecipient = malloc(sizeof(transactionout));
    *senderToRecipient = new_transactionout(t->recipientKey, t->value, t->id);
    transactionout *senderToSender = malloc(sizeof(transactionout));
    *senderToRecipient = new_transactionout(t->senderKey.key, remainder, t->id);

    put_val_on_list(t->outputs, senderToRecipient);
    put_val_on_list(t->outputs, senderToSender);

    temp = t->outputs->first;
    while(temp) {
        put_val_on_hashmap(outputs, ((transactionout*)temp->val)->id, temp->val);
        temp = temp->next;
    }
    temp = t->inputs->first;
    while(temp) {
        if(!((transactionin*)temp->val)->output) {
          rem_key_from_hashmap(outputs, ((transactionin*)temp->val)->output->id);
        }
        temp = temp->next;
    }
    return true;
}
#endif