//
// cloRedis reply class implementation 
// RedisReply is an encapsulation of redisReply struct in hiredis
// version: 1.0 
// Copyright (C) 2018 James Wei (weijianlhp@163.com). All rights reserved
//

#include <string.h>
#include <memory>
#include "internal/log.h"
#include "reply.h"

namespace cloris {

RedisReply::RedisReply() {
    cLog(TRACE, "RedisReply constructor..."); 
    Init(NULL, true, STATE_ERROR_INVOKE, NULL);
}

RedisReply::RedisReply(RedisReply&& reply) {
    cLog(TRACE, "RedisReply move constructor..."); 
    Init(reply.mutable_reply(), reply.reclaim(), reply.err_state(), reply.err_msg());
}

RedisReply& RedisReply::operator=(RedisReply&& reply) {
    cLog(TRACE, "RedisReply move assignment..."); 
    RemoveOldState();
    Init(reply.mutable_reply(), reply.reclaim(), reply.err_state(), reply.err_msg());
    return *this;
}

RedisReply::RedisReply(redisReply* reply, bool reclaim, ERR_STATE state, const char* err_msg) {
    cLog(TRACE, "RedisReply constructor..."); 
    Init(reply, reclaim, state, err_msg);
}

RedisReply::~RedisReply() {
    cLog(TRACE, "RedisReply destructor..."); 
    RemoveOldState();
}

void RedisReply::RemoveOldState() {
    if (reply_ && reclaim_) {
        freeReplyObject(reply_);
        reply_ = NULL;
    }
}

void RedisReply::UpdateErrMsg(const char* err_msg) {
    if (!err_msg) {
        return;
    }
    size_t len = strlen(err_msg);
    len = (len < sizeof(err_msg_)) ? len : (sizeof(err_msg_) - 1);
    memcpy(err_msg_, err_msg, len);
    err_msg_[len + 1] = '\0';
}

void RedisReply::Init(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg) {
    reply_ = rep;
    reclaim_ = reclaim;
    err_state_ = state;
    UpdateErrMsg(err_msg);
}

void RedisReply::Update(redisReply* rep, bool reclaim, ERR_STATE state, const char* err_msg) {
    RemoveOldState();
    Init(rep, reclaim, state, err_msg);
}

std::string RedisReply::toString() const {
    std::string value("");
    if (reply_) {
        switch (reply_->type) {
            case REDIS_REPLY_STRING:
            case REDIS_REPLY_STATUS:
            case REDIS_REPLY_ERROR:
                value.assign(reply_->str, reply_->len);
                break;
            default:
                ;
        }
    }
    return value;
}

int32_t RedisReply::toInt32() const {
    int32_t value(0);
    if (reply_) {
        if (reply_->type == REDIS_REPLY_INTEGER) {
            value = reply_->integer;
        } else if (reply_->type == REDIS_REPLY_STRING) {
            value = atoi(reply_->str);
        }
    }
    return value;
}

int64_t RedisReply::toInt64() const {
    int64_t value(-1);
    if (reply_) {
        if (reply_->type == REDIS_REPLY_INTEGER) {
            value = reply_->integer;
        } else if (reply_->type == REDIS_REPLY_STRING) {
            value = atol(reply_->str);
        }
    }
    return value;
}

bool RedisReply::error() const {  
    return (!reply_ || (reply_->type == REDIS_REPLY_ERROR));
}

bool RedisReply::ok() const {  
    return (reply_ && (reply_->type != REDIS_REPLY_ERROR)); 
}

std::string RedisReply::err_str() const {
    std::string value("");
    if (reply_) {
        if (reply_->type == REDIS_REPLY_ERROR) {
            value = reply_->str;
        }
    } else {
        value = this->err_msg_;
    }
    return value;
}

int RedisReply::type() const {
    if (reply_) {
        return reply_->type;
    } else {
        return REDIS_REPLY_ERROR;
    }
}

bool RedisReply::is_nil() const {
    return (reply_ && (reply_->type == REDIS_REPLY_NIL));
}

bool RedisReply::is_string() const {
    return (reply_ && (reply_->type == REDIS_REPLY_STRING));
}

bool RedisReply::is_int() const {
    return (reply_ && (reply_->type == REDIS_REPLY_INTEGER));
}

bool RedisReply::is_array() const {
    return (reply_ && (reply_->type == REDIS_REPLY_ARRAY));
}

size_t RedisReply::size() const {
    if (reply_ && (reply_->type == REDIS_REPLY_ARRAY)) {
        return reply_->elements;
    } else {
        return 0;
    }
}

RedisReply RedisReply::get(size_t index) {
    return (*this)[index];
}

RedisReply RedisReply::operator[](size_t index) {
    RedisReply reply;
    if (!reply_ || (reply_->type != REDIS_REPLY_ARRAY) || (index >= reply_->elements)) {
        cLog(ERROR, "reply type is not array");
        return RedisReply();
    } else {
        return RedisReply(reply_->element[index], false, STATE_OK, "");
    }
}

} // namespace cloris

