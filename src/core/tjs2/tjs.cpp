//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "tTJS" script language API class implementation
//---------------------------------------------------------------------------

#include "tjsCommHead.h"

#include <map>
#include <cassert>
#if !MY_USE_MINLIB
#include <fmt/printf.h>
#endif
#include "tjs.h"
#include "tjsScriptBlock.h"
#include "tjsArray.h"
#include "tjsDictionary.h"
#include "tjsDate.h"
#include "tjsMath.h"
#include "tjsException.h"
#include "tjsInterCodeExec.h"
#include "tjsScriptCache.h"
#include "tjsLex.h"
#include "tjsHashSearch.h"
#include "tjsRandomGenerator.h"
#include "tjsGlobalStringMap.h"
#include "tjsDebug.h"
#include "tjsByteCodeLoader.h"
#include "tjsBinarySerializer.h"
#include "tjsRegExp.h"

namespace TJS {
#ifndef TJS_NO_REGEXP

    extern iTJSDispatch2 *TJSCreateRegExpClass();
// to avoid to include large regexp library header
#endif
    //---------------------------------------------------------------------------
    // TJS Version
    //---------------------------------------------------------------------------
    const tjs_int TJSVersionMajor = 2;
    const tjs_int TJSVersionMinor = 4;
    const tjs_int TJSVersionRelease = 28;
    const tjs_int TJSVersionHex = TJSVersionMajor * 0x1000000 +
        TJSVersionMinor * 0x10000 + TJSVersionRelease;

    const char TJSCompiledDate[] = "" __DATE__ " " __TIME__;

    // first empty literal string is to avoid a compile error with bcc
    // which can not process directly L __DATE__ as a pre-processer
    // wide literal string.
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // global options
    //---------------------------------------------------------------------------
    bool TJSEvalOperatorIsOnGlobal = false;
    // Post-! operator (evaluate expression) is to be executed on
    // "this" context since TJS2 version 2.4.1
    bool TJSWarnOnNonGlobalEvalOperator = false;
    // Output warning against non-local post-! operator.
#ifdef _DEBUG
    bool TJSEnableDebugMode = true;
#else
    bool TJSEnableDebugMode = false;
#endif
    // Enable TJS2 Debugging support. Enabling this may make the
    // program somewhat slower and using more memory.
    // Do not use this mode unless you want to debug the program.
    bool TJSWarnOnExecutionOnDeletingObject = false;
    // Output warning against running code on context of
    // deleting-in-progress object. This is available only the Debug
    // mode is enabled.
    bool TJSUnaryAsteriskIgnoresPropAccess = false;
    // Unary '*' operator means accessing property object directly
    // without normal property access, if this options is set true.
    // This is replaced with '&' operator since TJS2 2.4.15. Turn true
    // for gaining old compatibility.
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // tTJSPPMap : value container for pre-processor
    //---------------------------------------------------------------------------
    class tTJSPPMap {
    public:
        tTJSHashTable<ttstr, tjs_int32> Values;
    };
    //---------------------------------------------------------------------------

#define TJS_GLOBAL_HASH_BITS 7

    //---------------------------------------------------------------------------
    // tTJS
    //---------------------------------------------------------------------------
    tTJS::tTJS() {
        // tTJS constructor
        RefCount = 1;
        ConsoleOutput = nullptr;
        PPValues = nullptr;

        // ensure variant array stack for function stack
        //	TJSVariantArrayStackAddRef();
        VariantArrayStack = new tTJSVariantArrayStack;

        // ensure hash table for reserved words
        TJSReservedWordsHashAddRef();

        // AddRef create global string map object
        TJSAddRefGlobalStringMap();

        // Create debugging-related objects
        if(TJSEnableDebugMode) {
            TJSAddRefObjectHashMap();
            TJSAddRefStackTracer();
        }

        // create script cache object
        Cache = new tTJSScriptCache(this);

        try {

            // push version value to pp value
            PPValues = new tTJSPPMap();
            PPValues->Values.Add(ttstr(TJS_W("version")), TJSVersionHex);

            // create the GLOBAL object
            Global = new tTJSCustomObject(TJS_GLOBAL_HASH_BITS);

            if(TJSObjectHashMapEnabled())
                TJSObjectHashSetType(Global, TJS_W("the global object"));

            // register some default classes to the GLOBAL
            iTJSDispatch2 *dsp;
            tTJSVariant val;

            // Array
            dsp = new tTJSArrayClass(); // TJSCreateArrayClass();
            val = tTJSVariant(dsp, nullptr);
            dsp->Release();
            Global->PropSet(TJS_MEMBERENSURE, TJS_W("Array"), nullptr, &val,
                            Global);

            // Dictionary
            dsp = new tTJSDictionaryClass();
            val = tTJSVariant(dsp, nullptr);
            dsp->Release();
            Global->PropSet(TJS_MEMBERENSURE, TJS_W("Dictionary"), nullptr,
                            &val, Global);

            // Date
            dsp = new tTJSNC_Date();
            val = tTJSVariant(dsp, nullptr);
            dsp->Release();
            Global->PropSet(TJS_MEMBERENSURE, TJS_W("Date"), nullptr, &val,
                            Global);

            // Math
            {
                iTJSDispatch2 *math;

                dsp = math = new tTJSNC_Math();
                val = tTJSVariant(dsp, nullptr);
                dsp->Release();
                Global->PropSet(TJS_MEMBERENSURE, TJS_W("Math"), nullptr, &val,
                                Global);

                // Math.RandomGenerator
                dsp = new tTJSNC_RandomGenerator();
                val = tTJSVariant(dsp, nullptr);
                dsp->Release();
                math->PropSet(TJS_MEMBERENSURE, TJS_W("RandomGenerator"),
                              nullptr, &val, math);
            }

            // Exception
            dsp = new tTJSNC_Exception();
            val = tTJSVariant(dsp, nullptr);
            dsp->Release();
            Global->PropSet(TJS_MEMBERENSURE, TJS_W("Exception"), nullptr, &val,
                            Global);
            // RegExp
            dsp = TJSCreateRegExpClass(); // the body is implemented
                                          // in tjsRegExp.cpp
            val = tTJSVariant(dsp, nullptr);
            dsp->Release();
            Global->PropSet(TJS_MEMBERENSURE, TJS_W("RegExp"), nullptr, &val,
                            Global);
        } catch(...) {
            Cleanup();

            throw;
        }
    }

    //---------------------------------------------------------------------------
    tTJS::~tTJS() {
        // tTJS destructor
        Cleanup();
    }

    //---------------------------------------------------------------------------
    void tTJS::Cleanup() {
        TJSVariantArrayStackCompactNow();
        //	TJSVariantArrayStackRelease();
        delete VariantArrayStack;
        VariantArrayStack = nullptr;

        if(Global)
            Global->Release(), Global = nullptr;

        delete PPValues;
        delete Cache;

        TJSReservedWordsHashRelease();

        TJSReleaseGlobalStringMap();

        TJSReleaseRegex();

        if(TJSEnableDebugMode) {
            TJSReleaseStackTracer();
            TJSReleaseObjectHashMap();
        }
    }

    //---------------------------------------------------------------------------
    void tTJS::AddRef() { RefCount++; }

    //---------------------------------------------------------------------------
    void tTJS::Release() {
        if(RefCount == 1) {
            delete this;
        } else {
            RefCount--;
        }
    }

    //---------------------------------------------------------------------------
    void tTJS::Shutdown() {
        TJSVariantArrayStackCompactNow();
        Global->Clear();
        if(Global)
            Global->Release(), Global = nullptr;
        if(Cache)
            delete Cache, Cache = nullptr;
    }

    //---------------------------------------------------------------------------
    iTJSDispatch2 *tTJS::GetGlobal() {
        Global->AddRef();
        return Global;
    }

    //---------------------------------------------------------------------------
    iTJSDispatch2 *tTJS::GetGlobalNoAddRef() const { return Global; }

    //---------------------------------------------------------------------------
    void tTJS::AddScriptBlock(tTJSScriptBlock *block) {
        ScriptBlocks.push_back(block);
    }

    //---------------------------------------------------------------------------
    void tTJS::RemoveScriptBlock(tTJSScriptBlock *block) {
        auto i = ScriptBlocks.begin();
        while(i != ScriptBlocks.end()) {
            if(*i == block) {
                ScriptBlocks.erase(i);
                break;
            }
            ++i;
        }
    }

    //---------------------------------------------------------------------------
    void tTJS::SetConsoleOutput(iTJSConsoleOutput *console) {
        // set a console output callback routine
        ConsoleOutput = console;
    }

    //---------------------------------------------------------------------------
    void tTJS::OutputToConsole(const tjs_char *msg) const {
        if(ConsoleOutput) {
            ConsoleOutput->Print(msg);
        }
    }

    //---------------------------------------------------------------------------
    void tTJS::OutputExceptionToConsole(const tjs_char *msg) const {
        if(ConsoleOutput) {
            ConsoleOutput->Print(msg);
        }
    }

    //---------------------------------------------------------------------------
    void tTJS::OutputToConsoleWithCentering(const tjs_char *msg,
                                            tjs_uint width) const {
        // this function does not matter whether msg includes ZENKAKU
        // characters ...
        if(!msg)
            return;
        auto len = (tjs_int)TJS_strlen(msg);
        tjs_int ns = ((tjs_int)width - len) / 2;
        if(ns <= 0) {
            OutputToConsole(msg);
        } else {
            auto *outbuf = new tjs_char[ns + len + 1];
            tjs_char *p = outbuf;
            while(ns--)
                *(p++) = TJS_W(' ');
            TJS_strcpy(p, msg);
            try {
                OutputToConsole(outbuf);
            } catch(...) {
                delete[] outbuf;
                throw;
            }

            delete[] outbuf;
        }
    }

    //---------------------------------------------------------------------------
    void tTJS::OutputToConsoleSeparator(const tjs_char *text,
                                        tjs_uint count) const {
        auto len = (tjs_int)TJS_strlen(text);
        auto *outbuf = new tjs_char[len * count + 1];
        tjs_char *p = outbuf;
        while(count--) {
            TJS_strcpy(p, text);
            p += len;
        }

        try {
            OutputToConsole(outbuf);
        } catch(...) {
            delete[] outbuf;
            throw;
        }

        delete[] outbuf;
    }

    //---------------------------------------------------------------------------
    void tTJS::Dump(tjs_uint width) const {
#if !MY_USE_MINLIB	
        ttstr version{ fmt::format("TJS version {}.{}.{} ({})", TJSVersionMajor,
                                   TJSVersionMinor, TJSVersionRelease,
                                   TJSCompiledDate) };
#else
	tjs_char version[100];
	swprintf((wchar_t *)version, sizeof(version)/sizeof(tjs_char), L"TJS version %d.%d.%d (%s)", TJSVersionMajor,
		TJSVersionMinor, TJSVersionRelease, TJSCompiledDate);
#endif
        OutputToConsoleSeparator(TJS_W("#"), width);
        OutputToConsoleWithCentering(TJS_W("TJS Context Dump"), width);
        OutputToConsoleSeparator(TJS_W("#"), width);
#if !MY_USE_MINLIB		
        OutputToConsole(version.c_str());
#else
        OutputToConsole(version);
#endif
        OutputToConsole(TJS_W(""));

        if(!ScriptBlocks.empty()) {
            std::vector<tTJSScriptBlock *>::const_iterator i;

#if !MY_USE_MINLIB
            ttstr buf{ fmt::format("Total {} script block(s)",
                                   ScriptBlocks.size()) };
            OutputToConsole(buf.c_str());
#else
			tjs_char buf[1024];
			swprintf((wchar_t *)buf, sizeof(buf)/sizeof(tjs_char), L"Total %d script block(s)", ScriptBlocks.size());
            OutputToConsole(buf);
#endif
            OutputToConsole(TJS_W(""));

            tjs_uint totalcontexts = 0;
            tjs_uint totalcodesize = 0;
            tjs_uint totaldatasize = 0;

            for(i = ScriptBlocks.begin(); i != ScriptBlocks.end(); i++) {
                tjs_uint n;
                const tjs_char *name = (*i)->GetName();

                ttstr title;
                if(name)
                    title = (*i)->GetNameInfo();
                else
                    title = TJS_W("(no-named script block)");

#if !MY_USE_MINLIB
                ttstr ptr{ fmt::format(" {}", static_cast<void *>(*i)) };
#else
				tjs_char ptr[256];
				swprintf((wchar_t *)ptr, sizeof(ptr)/sizeof(tjs_char), L" 0x%p", (*i));
#endif

                title += ptr;

                OutputToConsole(title.c_str());

                n = (*i)->GetContextCount();
                totalcontexts += n;
#if !MY_USE_MINLIB
                buf = { fmt::format("\tCount of contexts      : {}", n) };

                OutputToConsole(buf.c_str());
#else
				swprintf((wchar_t *)buf, sizeof(buf)/sizeof(tjs_char), L"\tCount of contexts      : %d", n);
                OutputToConsole(buf);
#endif

                n = (*i)->GetTotalVMCodeSize();
                totalcodesize += n;
#if !MY_USE_MINLIB
                buf = { fmt::format("\tVM code area size      : {} words", n) };

                OutputToConsole(buf.c_str());
#else
				swprintf((wchar_t *)buf, sizeof(buf)/sizeof(tjs_char), L"\tVM code area size      : %d words", n);
                OutputToConsole(buf);
#endif

                n = (*i)->GetTotalVMDataSize();
                totaldatasize += n;
#if !MY_USE_MINLIB
                buf = { fmt::format("\tVM constant data count : {}", n) };

                OutputToConsole(buf.c_str());
#else
				swprintf((wchar_t *)buf, sizeof(buf)/sizeof(tjs_char), L"\tVM constant data count : %d", n);
                OutputToConsole(buf);
#endif

                OutputToConsole(TJS_W(""));
            }
#if !MY_USE_MINLIB
            buf = { fmt::format("Total count of contexts      : {}",
                                totalcontexts) };
            OutputToConsole(buf.c_str());
            buf = { fmt::format("Total VM code area size      : {} words",
                                totalcodesize) };
            OutputToConsole(buf.c_str());
            buf = { fmt::format("Total VM constant data count : {}",
                                totaldatasize) };
            OutputToConsole(buf.c_str());
#else
			swprintf((wchar_t *)buf, sizeof(buf)/sizeof(tjs_char), L"Total count of contexts      : %d", totalcontexts);
			OutputToConsole(buf);
			swprintf((wchar_t *)buf, sizeof(buf)/sizeof(tjs_char), L"Total VM code area size      : %d words", totalcodesize);
			OutputToConsole(buf);
			swprintf((wchar_t *)buf, sizeof(buf)/sizeof(tjs_char), L"Total VM constant data count : %d", totaldatasize);
			OutputToConsole(buf);
#endif

            OutputToConsole(TJS_W(""));

            for(i = ScriptBlocks.begin(); i != ScriptBlocks.end(); i++) {

                OutputToConsoleSeparator(TJS_W("-"), width);
                const tjs_char *name = (*i)->GetName();

                ttstr title;
                if(name)
                    title = (*i)->GetNameInfo();
                else
                    title = TJS_W("(no-named script block)");

#if !MY_USE_MINLIB
                ttstr ptr{ fmt::format(" {}", static_cast<void *>(*i)) };
#else
				tjs_char ptr[256];
				swprintf((wchar_t *)ptr, sizeof(ptr)/sizeof(tjs_char), L" 0x%p", (*i));
#endif

                title += ptr;

                OutputToConsoleWithCentering(title.c_str(), width);

                OutputToConsoleSeparator(TJS_W("-"), width);

                (*i)->Dump();

                OutputToConsole(TJS_W(""));
                OutputToConsole(TJS_W(""));
            }
        } else {
            OutputToConsole(TJS_W(""));
            OutputToConsole(TJS_W("There are no script blocks in the system."));
        }
    }

    //---------------------------------------------------------------------------
    void tTJS::ExecScript(const tjs_char *script, tTJSVariant *result,
                          iTJSDispatch2 *context, const tjs_char *name,
                          tjs_int lineofs) {
        TJSSetFPUE();
        if(Cache)
            Cache->ExecScript(script, result, context, name, lineofs);
    }

    //---------------------------------------------------------------------------
    void tTJS::ExecScript(const ttstr &script, tTJSVariant *result,
                          iTJSDispatch2 *context, const ttstr *name,
                          tjs_int lineofs) {
        TJSSetFPUE();
        if(Cache)
            Cache->ExecScript(script, result, context, name, lineofs);
    }

    //---------------------------------------------------------------------------
    void tTJS::EvalExpression(const tjs_char *expression, tTJSVariant *result,
                              iTJSDispatch2 *context, const tjs_char *name,
                              tjs_int lineofs) {
        TJSSetFPUE();
        if(Cache)
            Cache->EvalExpression(expression, result, context, name, lineofs);
    }

    //---------------------------------------------------------------------------
    void tTJS::EvalExpression(const ttstr &expression, tTJSVariant *result,
                              iTJSDispatch2 *context, const ttstr *name,
                              tjs_int lineofs) {
        TJSSetFPUE();
        if(Cache)
            Cache->EvalExpression(expression, result, context, name, lineofs);
    }

    //---------------------------------------------------------------------------
    void tTJS::SetPPValue(const tjs_char *name, tjs_int32 value) {
        PPValues->Values.Add(ttstr(name), value);
    }

    //---------------------------------------------------------------------------
    tjs_int32 tTJS::GetPPValue(const tjs_char *name) {
        tjs_int32 *f = PPValues->Values.Find(ttstr(name));
        if(!f)
            return 0;
        return *f;
    }

    //---------------------------------------------------------------------------
    void tTJS::DoGarbageCollection() {
        // do garbage collection
        TJSVariantArrayStackCompactNow();
        TJSCompactStringHeap();
    }

    //---------------------------------------------------------------------------
    // for Bytecode
    void tTJS::LoadByteCode(const tjs_uint8 *buff, size_t len,
                            tTJSVariant *result, iTJSDispatch2 *context,
                            const tjs_char *name) {
        TJSSetFPUE();
        if(Cache)
            Cache->LoadByteCode(buff, len, result, context, name);
    }

    //---------------------------------------------------------------------------
    bool tTJS::LoadByteCode(class tTJSBinaryStream *stream, tTJSVariant *result,
                            iTJSDispatch2 *context, const tjs_char *name) {
        bool ret = false;
        tjs_uint8 *buff = nullptr;
        try {
            tjs_uint64 streamlen = stream->GetSize();
            if(streamlen >= tTJSScriptBlock::BYTECODE_FILE_TAG_SIZE) {
                tjs_uint8 header[tTJSScriptBlock::BYTECODE_FILE_TAG_SIZE];
                stream->Read(header, tTJSScriptBlock::BYTECODE_FILE_TAG_SIZE);
                if(tTJSByteCodeLoader::IsTJS2ByteCode(header)) {
                    stream->Seek(0, TJS_BS_SEEK_SET);
                    buff = new tjs_uint8[static_cast<unsigned int>(streamlen)];
                    stream->Read(buff, static_cast<tjs_uint>(streamlen));
                    LoadByteCode(buff, static_cast<size_t>(streamlen), result,
                                 context, name);
                    ret = true;
                } else {
                    assert(tTJSScriptBlock::BYTECODE_FILE_TAG_SIZE ==
                           tTJSBinarySerializer::HEADER_LENGTH);
                    if(result != nullptr &&
                       tTJSBinarySerializer::IsBinary(header)) {
                        tTJSBinarySerializer binload;
                        tTJSVariant *var = binload.Read(stream);
                        if(var) {
                            *result = *var;
                            delete var;
                            ret = true;
                        }
                    }
                }
            }
        } catch(...) {
            delete[] buff;
            throw;
        }
        delete[] buff;
        return ret;
    }

    //---------------------------------------------------------------------------
    bool tTJS::LoadBinaryDictionayArray(class tTJSBinaryStream *stream,
                                        tTJSVariant *result) {
        if(result == nullptr)
            return false;

        bool ret = false;
        try {
            tjs_uint64 streamlen = stream->GetSize();
            if(streamlen >= tTJSBinarySerializer::HEADER_LENGTH) {
                tjs_uint8 header[tTJSBinarySerializer::HEADER_LENGTH];
                stream->Read(header, tTJSBinarySerializer::HEADER_LENGTH);
                if(tTJSBinarySerializer::IsBinary(header)) {
                    tTJSBinarySerializer binload;
                    tTJSVariant *var = binload.Read(stream);
                    if(var) {
                        *result = *var;
                        delete var;
                        ret = true;
                    }
                }
            }
        } catch(...) {
            throw;
        }
        return ret;
    }

    //---------------------------------------------------------------------------
    void tTJS::CompileScript(const tjs_char *script,
                             class tTJSBinaryStream *output,
                             bool isresultneeded, bool outputdebug,
                             bool isexpression, const tjs_char *name,
                             tjs_int lineofs) {
        auto *blk = new tTJSScriptBlock(this);
        try {
            if(name)
                blk->SetName(name, lineofs);
            blk->Compile(script, isexpression, isresultneeded, outputdebug,
                         output);
        } catch(...) {
            blk->Release();
            throw;
        }
        blk->Release();
    }
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // TextStream Creation
    //---------------------------------------------------------------------------
    iTJSTextReadStream *TJSDefCreateTextStreamForRead(const tTJSString &name,
                                                      const tTJSString &mode) {
        return nullptr;
    }

    iTJSTextWriteStream *
    TJSDefCreateTextStreamForWrite(const tTJSString &name,
                                   const tTJSString &mode) {
        return nullptr;
    }

    tTJSBinaryStream *TJSDefCreateBinaryStreamForRead(const tTJSString &name,
                                                      const tTJSString &mode) {
        return nullptr;
    }

    tTJSBinaryStream *TJSDefCreateBinaryStreamForWrite(const tTJSString &name,
                                                       const tTJSString &mode) {
        return nullptr;
    }

    //---------------------------------------------------------------------------
    iTJSTextReadStream *(*TJSCreateTextStreamForRead)(const tTJSString &name,
                                                      const tTJSString &mode) =
        TJSDefCreateTextStreamForRead;

    iTJSTextWriteStream *(*TJSCreateTextStreamForWrite)(
        const tTJSString &name,
        const tTJSString &mode) = TJSDefCreateTextStreamForWrite;

    tTJSBinaryStream *(*TJSCreateBinaryStreamForRead)(const tTJSString &name,
                                                      const tTJSString &mode) =
        TJSDefCreateBinaryStreamForRead;

    tTJSBinaryStream *(*TJSCreateBinaryStreamForWrite)(const tTJSString &name,
                                                       const tTJSString &mode) =
        TJSDefCreateBinaryStreamForWrite;
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // tTJSBinaryStream
    //---------------------------------------------------------------------------

    void tTJSBinaryStream::SetEndOfStorage() { TJS_eTJSError(TJSWriteError); }

    //---------------------------------------------------------------------------
    tjs_uint64 tTJSBinaryStream::GetSize() {
        tjs_uint64 orgpos = GetPosition();
        tjs_uint64 size = Seek(0, TJS_BS_SEEK_END);
        Seek(orgpos, SEEK_SET);
        return size;
    }

    //---------------------------------------------------------------------------
    tjs_uint64 tTJSBinaryStream::GetPosition() { return Seek(0, SEEK_CUR); }

    //---------------------------------------------------------------------------
    void tTJSBinaryStream::SetPosition(tjs_uint64 pos) {
        if(pos != Seek(pos, TJS_BS_SEEK_SET))
            TJS_eTJSError(TJSSeekError);
    }

    //---------------------------------------------------------------------------
    void tTJSBinaryStream::ReadBuffer(void *buffer, tjs_uint read_size) {
        if(Read(buffer, read_size) != read_size) {
            TJS_eTJSError(TVPGetMessageByLocale("err_read_error"));
        }
    }

    //---------------------------------------------------------------------------
    void tTJSBinaryStream::WriteBuffer(const void *buffer,
                                       tjs_uint write_size) {
        if(Write(buffer, write_size) != write_size)
            TJS_eTJSError(TJSWriteError);
    }

    //---------------------------------------------------------------------------
    tjs_uint64 tTJSBinaryStream::ReadI64LE() {
#if TJS_HOST_IS_BIG_ENDIAN
        tjs_uint8 buffer[8];
        ReadBuffer(buffer, 8);
        tjs_uint64 ret = 0;
        for(tjs_int i = 0; i < 8; i++)
            ret += (tjs_uint64)buffer[i] << (i * 8);
        return ret;
#else
        tjs_uint64 temp;
        ReadBuffer(&temp, 8);
        return temp;
#endif
    }

    //---------------------------------------------------------------------------
    tjs_uint32 tTJSBinaryStream::ReadI32LE() {
#if TJS_HOST_IS_BIG_ENDIAN
        tjs_uint8 buffer[4];
        ReadBuffer(buffer, 4);
        tjs_uint32 ret = 0;
        for(tjs_int i = 0; i < 4; i++)
            ret += (tjs_uint32)buffer[i] << (i * 8);
        return ret;
#else
        tjs_uint32 temp;
        ReadBuffer(&temp, 4);
        return temp;
#endif
    }

    //---------------------------------------------------------------------------
    tjs_uint16 tTJSBinaryStream::ReadI16LE() {
#if TJS_HOST_IS_BIG_ENDIAN
        tjs_uint8 buffer[2];
        ReadBuffer(buffer, 2);
        tjs_uint16 ret = 0;
        for(tjs_int i = 0; i < 2; i++)
            ret += (tjs_uint16)buffer[i] << (i * 8);
        return ret;
#else
        tjs_uint16 temp;
        ReadBuffer(&temp, 2);
        return temp;
#endif
    }

    tjs_uint8 tTJSBinaryStream::ReadI8LE() {
        tjs_uint8 temp;
        ReadBuffer(&temp, 1);
        return temp;
    }
} // namespace TJS
