//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Name Space Processing
//---------------------------------------------------------------------------
#ifndef tjsNamespaceH
#define tjsNamespaceH

#include "tjsVariant.h"
#include <vector>

using namespace std;

namespace TJS {
    //---------------------------------------------------------------------------
    class iTJSDispatch;

    //---------------------------------------------------------------------------
    // tTJSLocalSymbolList
    //---------------------------------------------------------------------------
    struct tTJSLocalSymbol {
        tjs_char *Name;
    };

    //---------------------------------------------------------------------------
    class tTJSLocalSymbolList {
        vector<tTJSLocalSymbol *> List;
        tjs_int LocalCountStart;
        tjs_int *StartWriteAddr;
        tjs_int *CountWriteAddr;

    public:
        tTJSLocalSymbolList(tjs_int LocalCount);

        ~tTJSLocalSymbolList();

        void SetWriteAddr(tjs_int *StartWriteAddr, tjs_int *CountWriteAddr);

        void Add(const tjs_char *name);

        tjs_int Find(const tjs_char *name);

        void Remove(const tjs_char *name);

        [[nodiscard]] tjs_int GetCount() const { return (tjs_int)List.size(); }

        // this count includes variable holder that is marked as
        // un-used
        [[nodiscard]] tjs_int GetLocalCountStart() const {
            return LocalCountStart;
        }

        [[nodiscard]] tjs_int *GetStartWriteAddr() const {
            return StartWriteAddr;
        }

        [[nodiscard]] tjs_int *GetCountWriteAddr() const {
            return CountWriteAddr;
        }
    };

    //---------------------------------------------------------------------------
    // tTJSLocalNamespace
    //---------------------------------------------------------------------------
    class tTJSLocalNamespace {
        vector<tTJSLocalSymbolList *> Levels;
        tjs_int MaxCount; // max count of local variables
        tjs_int CurrentCount; // current local variable count
        tjs_int *MaxCountWriteAddr;

    public:
        tTJSLocalNamespace();

        ~tTJSLocalNamespace();

        void SetMaxCountWriteAddr(tjs_int *MaxCountWriteAddr);

        tjs_int GetCount();

        [[nodiscard]] tjs_int GetMaxCount() const { return MaxCount; }

        tjs_int Find(const tjs_char *name);

        tjs_int GetLevel();

        void Add(const tjs_char *name);

        void Remove(const tjs_char *name);

        void Commit();

        tTJSLocalSymbolList *GetTopSymbolList();

        void Push();

        void Pop();

        void Clear(); // all clear
    };

    //---------------------------------------------------------------------------
    // tTJSLocalNamespaceAutoPushPop
    //---------------------------------------------------------------------------
    class tTJSLocalNamespaceAutoPushPop {
        tTJSLocalNamespace *Space;

    public:
        tTJSLocalNamespaceAutoPushPop(tTJSLocalNamespace *space) {
            Space = space;
            Space->Push();
        }

        ~tTJSLocalNamespaceAutoPushPop() { Space->Pop(); }
    };

    //---------------------------------------------------------------------------
    // tTJSLocalNamespaceAutoClass
    //---------------------------------------------------------------------------
    class tTJSLocalNamespaceAutoClass {
        // create namespace if necessary
        tTJSLocalNamespace *Space;
        bool SpaceCreated;

    public:
        tTJSLocalNamespaceAutoClass(tTJSLocalNamespace *space) {
            Space = space;
            if(Space == nullptr) {
                Space = new tTJSLocalNamespace;
                SpaceCreated = true;
            } else {
                SpaceCreated = false;
            }
            Space->Push();
        }

        ~tTJSLocalNamespaceAutoClass() {
            Space->Pop();
            if(SpaceCreated)
                delete Space;
        }

        tTJSLocalNamespace *GetNamespace() { return Space; }
    };
    //---------------------------------------------------------------------------

} // namespace TJS
#endif
