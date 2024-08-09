// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define CHECK_ERROR(_op_) (_op_); if (FAILED(m_result)) { return; }

#define FIELD_TYPE_ERROR(_field_,_mark_) SetError(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, (_field_), (_mark_)); return
#define FIELD_TYPE_ERROR_IF(_condition_,_field_,_mark_) if (_condition_) { FIELD_TYPE_ERROR(_field_,_mark_); }

#define FIELD_MISSING_ERROR(_field_) SetError(WINGET_CONFIG_ERROR_MISSING_FIELD, (_field_)); return
#define FIELD_MISSING_ERROR_IF(_condition_,_field_) if (_condition_) { FIELD_MISSING_ERROR(_field_); }

#define FIELD_VALUE_ERROR(_field_,_value_,_mark_) SetError(WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, (_field_), (_mark_), (_value_)); return
#define FIELD_VALUE_ERROR_IF(_condition_,_field_,_value_,_mark_) if (_condition_) { FIELD_VALUE_ERROR(_field_,_value_,_mark_); }
