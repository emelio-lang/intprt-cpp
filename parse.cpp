/* ========================================================================
   $File: parse.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"
#include "util.h"

#define NOW p.tknvals[p.idx]
#define NEXT p.tknvals[p.idx+1]
#define NEXT2 p.tknvals[p.idx+2]
#define PASER(type, name)

// {!MONAD <> e <> t <> u <> {const pair<#t, ParserFlow&> tmp1 = #u(p); #e = tmp1.first;    /*p = tmp1.second;*/}!}
// {!MONAD_P <> e <> t <> u <> {const pair<#t, ParserFlow&> tmp1 = #u(p); #e = &tmp1.first; /*p = tmp1.second;*/}!}
// {!MONAD_F <> e <> t <> u <> {const pair<#t, ParserFlow&> tmp1 = #u(p); #e(tmp1.first);   /*p = tmp1.second;*/}!}
// {!SKIP <> s <> if (p.tknvals[p.idx] == #s) p.idx++; else cout << "Expects " << #s << " but " << p.tknvals[p.idx] << endl; !}
// {!PARSE <> type <> name <> pair<#type, ParserFlow&> #name(ParserFlow& p) !}

{!MONAD <> e <> t <> u <> {#e = #u(p);    /*p = tmp1.second;*/}!}
{!MONAD_P <> e <> t <> u <> {#e = &#u(p); /*p = tmp1.second;*/}!}
{!MONAD_F <> e <> t <> u <> {#e(#u(p));   /*p = tmp1.second;*/}!}
{!SKIP <> s <> if (p.tknvals[p.idx] == #s) p.idx++; else cout << "Expects " << #s << " but " << p.tknvals[p.idx] << endl; !}
{!PARSE <> type <> name <> #type #name(ParserFlow& p) !}

{- PARSE <> Literal <> literal -}
{
    Literal l;
    l.val = NOW;
    p.idx++;
    
    return l;
}

{- PARSE <> unique_ptr<Code> <> argument -}
;


TypeSignature named_ts(ParserFlow& p);
TypeSignature type_signature(ParserFlow& p);

{- PARSE <> TypeSignature <> named_ts -}
{
    TypeSignature typesig = make_shared<TypeProduct>(TypeProduct {});
    while (true) {
        PURES(TypeProduct)(typesig)->names.emplace_back(NOW);
        p.idx++;
        {- SKIP <> ":" -}
        TypeSignature tmp;
        {- MONAD <> tmp <> TypeSignature <> type_signature -}
        PURES(TypeProduct)(typesig)->products.emplace_back(tmp);
        if (NOW == ")") {
            return typesig;
        }
    }
}

// TODO: 関数分けてみても良いかも
// TypeSignature 種類
// And = Ty Ty Ty ...
// Or  = Ty | Ty | *Ty ...
// Fn  = Ty - Ty - Ty -> Ty
// Ty  = ( Ty )
// Ty  = name1:Ty name2:Ty ...
// Ty  = String < Ty ... >
// Ty  = String
{- PARSE <> TypeSignature <> type_signature -}
{
    TypeSignature typesig;
    enum { PRODUCT, SUM, FUNCTION, ATOM, PATOM, PARAM } type;

    if (NEXT == ":") {
        TypeSignature tmp;
        {- MONAD <> tmp <> TypeSignature <> named_ts -}
        return tmp;
    }
    
    if (NOW == "(") {
        type = PATOM;
        p.idx++;
    } else if (NOW == "*") {
        p.idx++;
        typesig = SpecialValue{NOW};
        p.idx++;
        return typesig;
    } else {
        typesig = NOW;
        p.idx++;
        return typesig;
    }

    {
        TypeSignature tmp;
        {- MONAD <> tmp <> TypeSignature <> type_signature -}
        if (NOW == ")" && type==PATOM) {
            {- SKIP <> ")" -}
            return tmp;
        } else if (NOW == ")") {
            // type == ATOM
            {- SKIP <> ")" -}
            return tmp;
        } else if (NOW == "|") {
            type = SUM;
            typesig = make_shared<TypeSum>(TypeSum { {tmp} });
            p.idx++;
        } else if (NOW == "-") {
            type = FUNCTION;
            typesig = make_shared<TypeFn>(TypeFn { {tmp} });
            p.idx++;
        } else if (NOW == "<") {
            type = PARAM;
            assert(MATCH(string)(tmp));
            typesig = make_shared<Parametered>(Parametered { PURE(string)(tmp), {} });
            p.idx++;
        } else {
            type = PRODUCT;
            typesig = make_shared<TypeProduct>(TypeProduct { {tmp} });
        }
    }
    
    while (true) {
        TypeSignature tmp;

        bool type_fn_to = false;
        if (type==FUNCTION && NOW==">") {
            type_fn_to = true;
            p.idx++;
        }
        // bool special_value = false;
        // if (type==SUM && NOW=="*") { // 特殊値
        //     special_value = true;
        //     {- SKIP <> "*" -}
        // }
        
        {- MONAD <> tmp <> TypeSignature <> type_signature -}
        switch (type) {
            case SUM:
                // if (special_value) {
                //     ASSERT(MATCH(string)(tmp), "特殊値なら文字列を指定してください " + to_string(tmp));
                //     PURES(TypeSum)(typesig)->add_type(SpecialValue {tmp});
                // } else {
                    PURES(TypeSum)(typesig)->add_type(tmp);
                // }
                break;
            case PRODUCT: PURES(TypeProduct)(typesig)->products.push_back(tmp);break;
            case FUNCTION:
                if (type_fn_to) {
                    PURES(TypeFn)(typesig)->to = tmp;
                    p.idx++;
                    return typesig;
                } else {
                    PURES(TypeFn)(typesig)->from.push_back(tmp);
                }
                break;
            case PARAM:
                PURES(Parametered)(typesig)->params.emplace_back(tmp);
                if (NOW == ">") {
                    {- SKIP <> ">" -}
                    return typesig;
                }
                break;
        }
        
        if (NOW == ")") {
            {- SKIP <> ")" -}
            return typesig;
        }
        
        switch (type) {
            case SUM:
            {- SKIP <> "|" -}
            break;
            case FUNCTION:
            {- SKIP <> "-" -}
            break;
        }
    }
}


{- PARSE <> shared_ptr<Lambda> <> lambda -}
{
    shared_ptr<Lambda> l(new Lambda);

    {- SKIP <> "(" -}

    // 引数
    if (NOW == "|") {
        {- SKIP <> "|" -}

        for (; p.idx < p.tknvals.size(); p.idx++) {
            if (NOW == "|") {
                {- SKIP <> "|" -}
                break;
            } else {
                l->argnames.emplace_back(NOW);
                l->argarities.emplace_back(-1);
                l->argqualities.emplace_back(ArgQuality {});

                if (NEXT == ":") {
                    p.idx++;
                    {- SKIP <> ":" -}
                    // TypeSignature tmp;
                    // {- MONAD <> tmp <> TypeSignature <> type_signature -}
                    // l->argtypes.emplace_back(tmp);
                    shared_ptr<Code> tmp;
                    {- MONAD <> tmp <> shared_ptr<Code> <> argument -}
                    l->cArgtypes.push_back(tmp);
                    
                } else {
                    p.idx++;
                }

                p.idx--;
// codegen4用
//                if (NEXT == "*") {
//                    // 再帰
//                    {- SKIP <> NOW -}
//                    l->argqualities.back().recursive = true;
//                }

//                // 種システム：酒の指定があればパース
//                if (NEXT == "(") {
//                    {- SKIP <> NOW -}
//                    {- SKIP <> "(" -}
//                    l->argarities.emplace_back(stoi(NOW));
//                    {- SKIP <> NOW -}
//                } else {
//                    l->argarities.emplace_back(-1);
//                }
            }
        }
    }

    {- MONAD <> l->body <> shared_ptr<Code> <> code -}

    {- SKIP <> ")" -}

    return l;
}

{- PARSE <> unique_ptr<Code> <> argument -}
{
    unique_ptr<Code> c (new Code);

    c->src.beg = next(p.tknvals.begin(), (int) p.idx);

//    {- SKIP <> "(" -}

    if (NOW == "(") {
        {- MONAD <> c->l <> shared_ptr<Lambda> <> lambda -}
    } else {
        {- MONAD <> c->lit <> Literal <> literal -}
    }

//    {- SKIP <> ")" -}

    c->src.end = next(p.tknvals.begin(), p.idx);
    
    return c;
}

// ( add 2 3 )
// ( (|x y| x + y) 2 3 )
{- PARSE <> unique_ptr<Code> <> code -}
{
    unique_ptr<Code> c (new Code);
    c->src.beg = next(p.tknvals.begin(), p.idx);

//    {- SKIP <> "(" -}
    if (NOW != ")") {
        if (NOW == ":") {
            {- SKIP <> ":" -}
            // 式のはじめに（適用する関数の位置に）型注釈がある場合（つまり、コンストラクタ）
            // NOTE: tnotationで記法を変更されている場合もあるので、ここはcodeとしてパースしておいて、reparse_types@reduction.cppでtnotationを適用しながらパースし直す
            {- MONAD <> c->cRawtype <> shared_ptr<code> <> argument -}
        } else if (NOW == "(") {
            {- MONAD <> c->l <> shared_ptr<Lambda> <> lambda -}
        } else {
            {- MONAD <> c->lit <> Literal <> literal -}
        }

        while (p.idx < p.tknvals.size()) {
            if (NOW == ")" || NOW == "") {
                break;
            }

            // NOTE: (↑と同様) tnotationで記法を変更されている場合もあるので、ここはcodeとしてパースしておいて、reparse_types@reduction.cppでtnotationを適用しながらパースし直す
            if (NOW == ":") {
                {- SKIP <> ":" -}
                
                shared_ptr<Code> newcode = shared_ptr<Code>(new Code);
                // TypeSignature tmp;
                // {- MONAD <> tmp <> TypeSignature <> type_signature -}
                shared_ptr<Code> tmp;
                {- MONAD <> tmp <> Code <> argument -}
//                newcode->rawtype = tmp;
//                c->args.emplace_back(newcode);
                newcode->cRawtype = tmp;
                c->args.emplace_back(newcode);
                continue;
            }

            {- MONAD_F <> c->args.emplace_back <> shared_ptr<Code> <> argument -}
        }
    }


//    {- SKIP <> ")" -}
    c->src.end = next(p.tknvals.begin(), p.idx);

    return c;
}
