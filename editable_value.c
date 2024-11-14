#include "editable_value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void EditableValue_PrintValue(EditableValue_t *editable)
{
	printf("Editable val: %s type: %d ", editable->name, editable->type);
	switch(editable->type)
	{
		case UINT8_T:
		{
			printf("val: %d lowerLimit: %d upperLimit: %d\n", editable->valPtr->u8, editable->ll.u8, editable->ul.u8);
			break;
		}
		case UINT16_T:
		{
			printf("val: %d lowerLimit: %d upperLimit: %d\n", editable->valPtr->u16,  editable->ll.u16, editable->ul.u16);
			break;
		}
		case UINT32_T:
		{
			printf("val: %d lowerLimit: %d upperLimit: %d\n", editable->valPtr->u32, editable->ll.u32, editable->ul.u32);
			break;
		}
		case DOUBLE:
		{
			printf("val: %f lowerLimit: %f upperLimit: %f\n", editable->valPtr->d, editable->ll.d, editable->ul.d);
			break;
		}
		case FLOAT:
		{
			printf("val: %f lowerLimit: %f upperLimit: %f\n", editable->valPtr->f, editable->ll.f, editable->ul.f);
			break;
		}
	}

	// Hexdump
	// char *b = (char *) v;
	// for (int i = 0; i < sizeof(EditableValue_t); i++)
	// {
	// 	printf("%02x ", *(b+i));
	// 	if (i % 16 == 0 && i != 0) printf("\n");
	// }
	// printf("\n");
}

void EditableValue_PrintList(EditableValueList_t *list)
{
	printf("Editable list name: %s len: %d\n", list->name, list->len);
	for (int i = 0; i < list->len; i++)
	{
		EditableValue_t *v = &(list->values[i]);
		printf("%d ", i);
		EditableValue_PrintValue(v);
	}
}

bool EditableValue_SetValue(EditableValue_t *editable, union EightByteData_u *value)
{
	// printf("%s\n", __FUNCTION__);
	TYPE t = editable->type;
	bool ret = true;
	switch(t)
	{
		case UINT8_T:
		{
			if (value->u8 < editable->ll.u8 || value->u8 > editable->ul.u8)
			{
				printf("Val %d not in range! %d %d\n", value->u8, editable->ll.u8, editable->ul.u8);
				ret = false;
				break;
			}
			editable->valPtr->u8 = value->u8;
			break;
		}
		case UINT16_T:
		{
			if (value->u16 < editable->ll.u16 || value->u16 > editable->ul.u16)
			{
				printf("Val %d not in range! %d %d\n", value->u16, editable->ll.u16, editable->ul.u16);
				ret = false;
				break;
			}
			editable->valPtr->u16 = value->u16;
			break;
		}
		case UINT32_T:
		{
			if (value->u32 < editable->ll.u32 || value->u32 > editable->ul.u32)
			{
				printf("Val %d not in range! %d %d\n", value->u32, editable->ll.u32, editable->ul.u32);
				ret = false;
				break;
			}
			editable->valPtr->u32 = value->u32;
			break;
		}
		case DOUBLE:
		{
			if (value->d < editable->ll.d || value->d > editable->ul.d)
			{
				printf("Val %f not in range! %f %f\n",value->d , editable->ll.u32, editable->ul.u32);
				ret = false;
				break;
			}
			editable->valPtr->d = value->d;
			break;
		}
		case FLOAT:
		{
			if (value->f < editable->ll.f || value->f > editable->ul.f)
			{
				printf("Val %d not in range! %d %d\n", value->f, editable->ll.f, editable->ul.f);
				ret = false;
				break;
			}
			editable->valPtr->f = value->f;
			break;
		}
		default:
		{
			return false;
		}
	}
	// printf("%s DONE\n", __FUNCTION__);
	return ret;
}

bool EditableValue_SetValueFromString(EditableValue_t *editable, char *valStr)
{
	// printf("%s\n", __FUNCTION__);
	union EightByteData_u tmpVal;
	switch(editable->type)
	{
		case UINT8_T:
		{
			tmpVal.u8 = (uint8_t) atoi(valStr);
			return EditableValue_SetValue(editable, &tmpVal);
			break;
		}
		case UINT16_T:
		{
			tmpVal.u16 = (uint16_t) atoi(valStr);
			return EditableValue_SetValue(editable, &tmpVal);
			break;
		}
		case UINT32_T:
		{
			tmpVal.u32 = (uint32_t) atoi(valStr);
			return EditableValue_SetValue(editable, &tmpVal);
			break;
		}
		case DOUBLE:
		{
			tmpVal.d = (double) atof(valStr);
			return EditableValue_SetValue(editable, &tmpVal);
			break;
		}
		case FLOAT:
		{
			tmpVal.f = (float) atof(valStr);
			return EditableValue_SetValue(editable, &tmpVal);
			break;
		}
		default:
		{
			return false;
		}
	}
	// printf("%s DONE\n", __FUNCTION__);
}

EditableValue_t* EditableValue_FindValueFromString(EditableValueList_t *l, char *name)
{
	EditableValue_t *editable = NULL;
	bool found = false;
	for (uint16_t i = 0; i < l->len; i++)
	{
		// printf("Looking for %s.. %s\n", name, l->values[i].name);
		if (strcmp(l->values[i].name, name) == 0)
		{
			editable = &(l->values[i]);
			found = true;
			break;
		}
	}
	if (!found)
	{
		printf("%s not found in value list!\n", name);
	}
	return editable;
}

uint16_t EditableValue_GetValueIdxFromString(EditableValueList_t *l, char *name)
{
	for (uint16_t i = 0; i < l->len; i++)
	{
		if (strcmp(l->values[i].name, name) == 0)
		{
			return i;
		}
	}
	printf("%s not found in value list!\n", name);
	return EDITABLE_VALUE_BAD_IDX;
}

bool EditableValue_FindAndSetValueFromString(EditableValueList_t *l, char *name, char *valStr)
{
	EditableValue_t *editable;
	bool found = false;
	for (uint16_t i = 0; i < l->len; i++)
	{
		// printf("Looking for %s.. %s\n", name, l->values[i].name);
		if (strcmp(l->values[i].name, name) == 0)
		{
			editable = &(l->values[i]);
			found = true;
			break;
		}
	}
	if (!found)
	{
		printf("%s not found in value list!\n", name);
		return found;
	}

	return EditableValue_SetValueFromString(editable, valStr);
}
