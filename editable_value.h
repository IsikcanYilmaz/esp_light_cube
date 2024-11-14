#ifndef EDITABLE_VALUE_H_
#define EDITABLE_VALUE_H_
#include <stdint.h>
#include <stdbool.h>

#define LONGEST_DATA_TYPE uint64_t // double 
#define COMMON_DATA_TYPE LONGEST_DATA_TYPE

#define EDITABLE_VALUE_BAD_IDX 0xffff

// Below takes the value name itself, not the pointer. the macro will make a pointer from the string 
#define EDITABLE_VALUE(valPtr, type, lowerLimit, upperLimit)  (EditableValue_t) { .name = #valPtr, .valPtr = &valPtr, .type = type, .lowerLimit = lowerLimit, .upperLimit = upperLimit }

typedef enum TYPE_
{
	UINT8_T,
	UINT16_T,
	UINT32_T,
	DOUBLE,
	FLOAT,
	TYPE_MAX
} TYPE;

union EightByteData_u
{
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	double d;
	float f;
};

typedef struct __attribute__((__packed__)) EditableValue_t_
{
	char *name;
	union EightByteData_u *valPtr;
	TYPE type;
	COMMON_DATA_TYPE lowerLimit; // TODO rm. these are deprecated
	COMMON_DATA_TYPE upperLimit;
	union EightByteData_u ll;
	union EightByteData_u ul;
} EditableValue_t;

typedef struct EditableValueList_t_
{
	char *name;
	EditableValue_t *values;
	uint16_t len;
} EditableValueList_t;

void EditableValue_PrintValue(EditableValue_t *editable);
void EditableValue_PrintList(EditableValueList_t *list);
bool EditableValue_SetValue(EditableValue_t *v, union EightByteData_u *value);
bool EditableValue_SetValueFromString(EditableValue_t *v, char *valStr);
EditableValue_t* EditableValue_FindValueFromString(EditableValueList_t *l, char *name);
bool EditableValue_FindAndSetValue(EditableValueList_t *l, char *name, union EightByteData_u *value);
bool EditableValue_FindAndSetValueFromString(EditableValueList_t *l, char *name, char *valStr);
uint16_t EditableValue_GetValueIdxFromString(EditableValueList_t *l, char *name);

#endif
