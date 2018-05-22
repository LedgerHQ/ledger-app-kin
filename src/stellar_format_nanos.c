/*******************************************************************************
 *   Ledger Stellar App
 *   (c) 2017-2018 Ledger
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 ********************************************************************************/
#ifndef TEST
#include "bolos_target.h"
#include "os.h"
#endif
#include "stellar_types.h"

#ifdef TARGET_NANOS

#include "stellar_format.h"
#include "stellar_api.h"
#include "string.h"

char opCaption[20];
char detailCaption[20];
char detailValue[67];

volatile format_function_t formatter;

void format_sequence_number(tx_context_t *txCtx) {
    strcpy(detailCaption, "Sequence Number");
    char sequenceNumber[22];
    print_uint(txCtx->txDetails.sequenceNumber, sequenceNumber);
    print_summary(sequenceNumber, detailValue, 5, 6);
    formatter = NULL;
}

void format_transaction_source(tx_context_t *txCtx) {
    strcpy(detailCaption, "Transaction Source");
    print_public_key(txCtx->txDetails.source, detailValue, 5, 6);
    formatter = &format_sequence_number;
}

void format_time_bounds_max_time(tx_context_t *txCtx) {
    strcpy(detailCaption, "Time Bounds To");
    print_uint(txCtx->txDetails.timeBounds.maxTime, detailValue);
    formatter = &format_transaction_source;
}

void format_time_bounds_min_time(tx_context_t *txCtx) {
    strcpy(detailCaption, "Time Bounds From");
    print_uint(txCtx->txDetails.timeBounds.minTime, detailValue);
    formatter = &format_time_bounds_max_time;
}

void format_time_bounds(tx_context_t *txCtx) {
    if (txCtx->txDetails.hasTimeBounds) {
        format_time_bounds_min_time(txCtx);
    } else {
        format_transaction_source(txCtx);
    }
}

void format_network(tx_context_t *txCtx) {
    strcpy(detailCaption, "Network");
    strcpy(detailValue, ((char *)PIC(NETWORK_NAMES[txCtx->txDetails.network])));
    formatter = &format_time_bounds;
}

void format_fee(tx_context_t *txCtx) {
    strcpy(detailCaption, "Fee");
    char nativeAssetCode[7];
    print_native_asset_code(txCtx->txDetails.network, nativeAssetCode);
    print_amount(txCtx->txDetails.fee, nativeAssetCode, detailValue);
    formatter = &format_network;
}

void format_memo(tx_context_t *txCtx) {
    strcpy(detailCaption, "Memo");
    char *suffix;
    switch (txCtx->txDetails.memo.type) {
        case MEMO_TYPE_ID: {
            suffix = " Id";
            strcpy(detailValue, txCtx->txDetails.memo.data);
            break;
        }
        case MEMO_TYPE_TEXT: {
            suffix = " Text";
            strcpy(detailValue, txCtx->txDetails.memo.data);
            break;
        }
        case MEMO_TYPE_HASH: {
            suffix = " Hash";
            print_summary(txCtx->txDetails.memo.data, detailValue, 8, 6);
            break;
        }
        case MEMO_TYPE_RETURN: {
            suffix = " Return";
            print_summary(txCtx->txDetails.memo.data, detailValue, 8, 6);
            break;
        }
        default: {
            suffix = "";
            strcpy(detailValue, "[none]");
        }
    }
    strcpy(detailCaption+4, suffix);
    formatter = &format_fee;
}

void format_confirm_transaction_details(tx_context_t *txCtx) {
    format_memo(txCtx);
}

void format_operation_source(tx_context_t *txCtx) {
    if (txCtx->opIdx == txCtx->opCount) {
        // last operation: show transaction details
        formatter = &format_confirm_transaction_details;
    } else {
        // next operation
        formatter = NULL;
    }
    if (txCtx->opDetails.sourcePresent) {
        strcpy(detailCaption, "Operation Source");
        print_public_key(txCtx->opDetails.source, detailValue, 5, 6);
    } else if (formatter) {
        formatter(txCtx);
    }
}

void format_bump_sequence(tx_context_t *txCtx) {
    strcpy(detailCaption, "Bump Sequence");
    print_int(txCtx->opDetails.op.bumpSequence.bumpTo, detailValue);
    formatter = &format_operation_source;
}

void format_inflation(tx_context_t *txCtx) {
    strcpy(opCaption, "Run Inflation");
    formatter = &format_operation_source;
}

void format_account_merge_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_public_key(txCtx->txDetails.source, detailValue, 12, 12);
    formatter = &format_operation_source;
}

void format_account_merge(tx_context_t *txCtx) {
    strcpy(detailCaption, "Merge Account");
    if (txCtx->opDetails.sourcePresent) {
        print_public_key(txCtx->opDetails.source, detailValue, 5, 6);
    } else {
        print_public_key(txCtx->txDetails.source, detailValue, 5, 6);
    }
    formatter = &format_account_merge_destination;
}

void format_manage_data_value(tx_context_t *txCtx) {
    strcpy(detailCaption, "Data Value");
    print_binary_summary(txCtx->opDetails.op.manageData.dataValue, detailValue, txCtx->opDetails.op.manageData.dataSize);
    formatter = &format_operation_source;
}

void format_manage_data(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.manageData.hasValue) {
        strcpy(detailCaption, "Set Data");
        formatter = &format_manage_data_value;
    } else {
        strcpy(detailCaption, "Remove Data");
        formatter = &format_operation_source;
    }
    print_summary(txCtx->opDetails.op.manageData.dataName, detailValue, 12, 12);
}

void format_allow_trust_trustee(tx_context_t *txCtx) {
    strcpy(detailCaption, "Account ID");
    print_public_key(txCtx->opDetails.op.allowTrust.trustee, detailValue, 12, 12);
    formatter = &format_operation_source;
}

void format_allow_trust(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.allowTrust.authorize) {
        strcpy(detailCaption, "Allow Trust");
    } else {
        strcpy(detailCaption, "Revoke Trust");
    }
    strcpy(detailValue, txCtx->opDetails.op.allowTrust.assetCode);
    formatter = &format_allow_trust_trustee;
}

void format_set_option_signer_weight(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.signer.weight) {
        strcpy(detailCaption, "Weight");
        print_uint(txCtx->opDetails.op.setOptions.signer.weight, detailValue);
        formatter = &format_operation_source;
    } else {
        format_operation_source(txCtx);
    }
}

void format_set_option_signer_detail(tx_context_t *txCtx) {
    switch (txCtx->opDetails.op.setOptions.signer.type) {
    case SIGNER_KEY_TYPE_ED25519: {
        strcpy(detailCaption, "Signer Public Key");
        print_public_key(txCtx->opDetails.op.setOptions.signer.data, detailValue, 12, 12);
        break;
    }
    case SIGNER_KEY_TYPE_HASH_X:
    case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
        strcpy(detailCaption, "Signer Hash");
        print_binary_summary(txCtx->opDetails.op.setOptions.signer.data, detailValue, 32);
        break;
    }
    }
    formatter = &format_set_option_signer_weight;
}

void format_set_option_signer(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.signerPresent) {
        if (txCtx->opDetails.op.setOptions.signer.weight) {
            strcpy(detailCaption, "Add Signer");
        } else {
            strcpy(detailCaption, "Remove Signer");
        }
        switch (txCtx->opDetails.op.setOptions.signer.type) {
        case SIGNER_KEY_TYPE_ED25519: {
            strcpy(detailValue, "Type Public Key");
            break;
        }
        case SIGNER_KEY_TYPE_HASH_X: {
            strcpy(detailValue, "Type Hash(x)");
            break;
        }
        case SIGNER_KEY_TYPE_PRE_AUTH_TX: {
            strcpy(detailValue, "Type Pre-Auth");
            break;
        }
        }
        formatter = &format_set_option_signer_detail;
    } else {
        format_operation_source(txCtx);
    }
}

void format_set_option_home_domain(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.homeDomainPresent) {
        strcpy(detailCaption, "Home Domain");
        strcpy(detailValue, txCtx->opDetails.op.setOptions.homeDomain);
        formatter = &format_set_option_signer;
    } else {
        format_set_option_signer(txCtx);
    }
}

void format_set_option_high_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.highThresholdPresent) {
        strcpy(detailCaption, "High Threshold");
        print_uint(txCtx->opDetails.op.setOptions.highThreshold, detailValue);
        formatter = &format_set_option_home_domain;
    } else {
        format_set_option_home_domain(txCtx);
    }
}

void format_set_option_medium_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.mediumThresholdPresent) {
        strcpy(detailCaption, "Medium Threshold");
        print_uint(txCtx->opDetails.op.setOptions.mediumThreshold, detailValue);
        formatter = &format_set_option_high_threshold;
    } else {
        format_set_option_high_threshold(txCtx);
    }
}

void format_set_option_low_threshold(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.lowThresholdPresent) {
        strcpy(detailCaption, "Low Threshold");
        print_uint(txCtx->opDetails.op.setOptions.lowThreshold, detailValue);
        formatter = &format_set_option_medium_threshold;
    } else {
        format_set_option_medium_threshold(txCtx);
    }
}

void format_set_option_master_weight(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.masterWeightPresent) {
        strcpy(detailCaption, "Master Weight");
        print_uint(txCtx->opDetails.op.setOptions.masterWeight, detailValue);
        formatter = &format_set_option_low_threshold;
    } else {
        format_set_option_low_threshold(txCtx);
    }
}

void format_set_option_set_flags(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.setFlagsPresent) {
        strcpy(detailCaption, "Set Flags");
        print_flags(txCtx->opDetails.op.setOptions.setFlags, detailValue, 0);
        formatter = &format_set_option_master_weight;
    } else {
        format_set_option_master_weight(txCtx);
    }
}

void format_set_option_clear_flags(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.clearFlagsPresent) {
        strcpy(detailCaption, "Clear Flags");
        print_flags(txCtx->opDetails.op.setOptions.clearFlags, detailValue, 0);
        formatter = &format_set_option_set_flags;
    } else {
        format_set_option_set_flags(txCtx);
    }
}

void format_set_option_inflation_destination(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.setOptions.inflationDestinationPresent) {
        strcpy(detailCaption, "Inflation Dest");
        print_public_key(txCtx->opDetails.op.setOptions.inflationDestination, detailValue, 5, 6);
        formatter = &format_set_option_clear_flags;
    } else {
        format_set_option_clear_flags(txCtx);
    }
}

void format_set_options(tx_context_t *txCtx) {
    format_set_option_inflation_destination(txCtx);
}

void format_change_trust_limit(tx_context_t *txCtx) {
    strcpy(detailCaption, "Trust Limit");
    if (txCtx->opDetails.op.changeTrust.limit == 9223372036854775807) {
        strcpy(detailValue, "[maximum]");
    } else {
        print_amount(txCtx->opDetails.op.changeTrust.limit, NULL, detailValue);
    }
    formatter = &format_operation_source;
}

void format_change_trust(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.changeTrust.limit) {
        strcpy(detailCaption, "Change Trust");
        formatter = &format_change_trust_limit;
    } else {
        strcpy(detailCaption, "Remove Trust");
        formatter = &format_operation_source;
    }
    print_asset_t(&txCtx->opDetails.op.changeTrust.asset, detailValue);
}

void format_manage_offer_sell(tx_context_t *txCtx) {
    strcpy(detailCaption, "Sell");
    print_amount(txCtx->opDetails.op.manageOffer.amount, txCtx->opDetails.op.manageOffer.selling.code, detailValue);
    formatter = &format_operation_source;
}

void format_manage_offer_price(tx_context_t *txCtx) {
    strcpy(detailCaption, "Price");
    uint64_t price = ((uint64_t)txCtx->opDetails.op.manageOffer.price.numerator * 10000000) /
        txCtx->opDetails.op.manageOffer.price.denominator;
    print_amount(price, txCtx->opDetails.op.manageOffer.buying.code, detailValue);
    formatter = &format_manage_offer_sell;
}

void format_manage_offer_buy(tx_context_t *txCtx) {
    strcpy(detailCaption, "Buy");
    print_asset_t(&txCtx->opDetails.op.manageOffer.buying, detailValue);
    formatter = &format_manage_offer_price;
}

void format_manage_offer(tx_context_t *txCtx) {
    if (!txCtx->opDetails.op.manageOffer.amount) {
        strcpy(detailCaption, "Remove Offer");
        print_uint(txCtx->opDetails.op.manageOffer.offerId, detailValue);
        formatter = &format_operation_source;
    } else {
        if (txCtx->opDetails.op.manageOffer.offerId) {
            strcpy(detailCaption, "Change Offer");
            print_uint(txCtx->opDetails.op.manageOffer.offerId, detailValue);
        } else {
            strcpy(detailCaption, "Create Offer");
            if (txCtx->opDetails.op.manageOffer.active) {
                strcpy(detailValue, "Type Active");
            } else {
                strcpy(detailValue, "Type Passive");
            }
        }
        formatter = &format_manage_offer_buy;
    }
}

void format_path_via(tx_context_t *txCtx) {
    if (txCtx->opDetails.op.pathPayment.pathLen) {
        strcpy(detailCaption, "Via");
        uint8_t i;
        for (i = 0; i < txCtx->opDetails.op.pathPayment.pathLen; i++) {
            asset_t asset = txCtx->opDetails.op.pathPayment.path[i];
            uint8_t len = strlen(detailValue);
            if (len) {
                strcpy(detailValue+len, ", ");
                len += 2;
            }
            strcpy(detailValue+len, asset.code);
        }
        formatter = &format_operation_source;
    } else {
        format_operation_source(txCtx);
    }
}

void format_path_receive(tx_context_t *txCtx) {
    strcpy(detailCaption, "Receive");
    print_amount(txCtx->opDetails.op.pathPayment.destAmount, txCtx->opDetails.op.pathPayment.destAsset.code, detailValue);
    formatter = &format_path_via;
}

void format_path_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_public_key(txCtx->opDetails.op.pathPayment.destination, detailValue, 12, 12);
    formatter = &format_path_receive;
}

void format_path_payment(tx_context_t *txCtx) {
    strcpy(detailCaption, "Send Max");
    print_amount(txCtx->opDetails.op.pathPayment.sendMax, txCtx->opDetails.op.pathPayment.sourceAsset.code, detailValue);
    formatter = &format_path_destination;
}

void format_create_account_amount(tx_context_t *txCtx) {
    strcpy(detailCaption, "Starting Balance");
    char nativeAssetCode[7];
    print_native_asset_code(txCtx->txDetails.network, nativeAssetCode);
    print_amount(txCtx->opDetails.op.createAccount.amount, nativeAssetCode, detailValue);
    formatter = &format_operation_source;
}

void format_create_account(tx_context_t *txCtx) {
    strcpy(detailCaption, "Create Account");
    print_public_key(txCtx->opDetails.op.createAccount.destination, detailValue, 12, 12);
    formatter = &format_create_account_amount;
}

void format_payment_destination(tx_context_t *txCtx) {
    strcpy(detailCaption, "Destination");
    print_public_key(txCtx->opDetails.op.payment.destination, detailValue, 12, 12);
    formatter = &format_operation_source;
}

void format_payment(tx_context_t *txCtx) {
    strcpy(detailCaption, "Send");
    print_amount(txCtx->opDetails.op.payment.amount, txCtx->opDetails.op.payment.asset.code, detailValue);
    formatter = &format_payment_destination;
}

const format_function_t formatters[12] = {
    &format_create_account,
    &format_payment,
    &format_path_payment,
    &format_manage_offer,
    &format_manage_offer,
    &format_set_options,
    &format_change_trust,
    &format_allow_trust,
    &format_account_merge,
    &format_inflation,
    &format_manage_data,
    &format_bump_sequence
};

void format_confirm_operation(tx_context_t *txCtx) {
    if (txCtx->opCount > 1) {
        strcpy(opCaption, "Operation ");
        print_uint(txCtx->opIdx, opCaption+strlen(opCaption));
        strcpy(opCaption+strlen(opCaption), " of ");
        print_uint(txCtx->opCount, opCaption+strlen(opCaption));
        formatter = ((format_function_t)PIC(formatters[txCtx->opDetails.type]));
    } else {
        ((format_function_t)PIC(formatters[txCtx->opDetails.type]))(txCtx);
    }
}

void format_confirm_transaction(tx_context_t *txCtx) {
    formatter = &format_confirm_operation;
}

void format_confirm_hash_detail(tx_context_t *txCtx) {
    strcpy(detailCaption, "Hash");
    print_binary_summary(txCtx->hash, detailValue, 32);
    formatter = &format_confirm_hash;
}

void format_confirm_hash_warning(tx_context_t *txCtx) {
    strcpy(detailCaption, "WARNING");
    strcpy(detailValue, "No details available");
    formatter = &format_confirm_hash_detail;
}

void format_confirm_hash(tx_context_t *txCtx) {
    formatter = &format_confirm_hash_warning;
}

#endif
