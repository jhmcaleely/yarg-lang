#include "value_cell.h"

#include "common.h"

bool assignToValueCellTarget(ValueCellTarget lhs, Value rhsValue) {
    if (lhs.cellType == NULL) {
        noLongerLiteralInt(&rhsValue);
        *lhs.value = rhsValue;
        return true;
    } else {
        Value promoted;
        if (isInitialisableType(lhs.cellType, rhsValue, &promoted)) {
            if (promoted.type == NULL)
            {
                noLongerLiteralInt(&rhsValue);
                *(lhs.value) = rhsValue;
            }
            else
            {
                *(lhs.value) = promoted;
            }
            return true;
        } else {
            return false;
        }
    }
}

bool initialiseValueCellTarget(ValueCellTarget lhs, Value rhsValue) {
    if (lhs.cellType == NULL) {
        noLongerLiteralInt(&rhsValue);
        *lhs.value = rhsValue;
        return true;
    } else {
        Value promoted;
        if (isInitialisableType(lhs.cellType, rhsValue, &promoted)) {
            if (promoted.type == NULL)
            {
                noLongerLiteralInt(&rhsValue);
                *(lhs.value) = rhsValue;
            }
            else
            {
                *(lhs.value) = promoted;
            }
            return true;
        } else {
            return false;
        }
    }
}