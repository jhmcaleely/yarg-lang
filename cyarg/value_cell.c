#include "value_cell.h"

#include "common.h"
#include "debug.h"
#include "yargobject.h"

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

void fprintValueCell(FILE* op, ValueCell* cell) {
    FPRINTMSG(op, "<vc:");
    fprintValue(op, cell->value);
    FPRINTMSG(op, "<");
    Value cellTypeValue = cell->cellType ? YARGTYPE_VAL((Obj*)cell->cellType) : NIL_VAL;
    fprintValue(op, cellTypeValue);
    FPRINTMSG(op, ">>");
}