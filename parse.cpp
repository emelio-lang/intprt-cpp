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
#define PASER(type, name)

// {!MONAD <> e <> t <> u <> {const pair<#t, ParserFlow&> tmp1 = #u(p); #e = tmp1.first;    /*p = tmp1.second;*/}!}
// {!MONAD_P <> e <> t <> u <> {const pair<#t, ParserFlow&> tmp1 = #u(p); #e = &tmp1.first; /*p = tmp1.second;*/}!}
// {!MONAD_F <> e <> t <> u <> {const pair<#t, ParserFlow&> tmp1 = #u(p); #e(tmp1.first);   /*p = tmp1.second;*/}!}
// {!SKIP <> s <> if (p.tknvals[p.idx] == #s) p.idx++; else cout << "Excepts " << #s << " but " << p.tknvals[p.idx] << endl; !}
// {!PARSE <> type <> name <> pair<#type, ParserFlow&> #name(ParserFlow& p) !}

{!MONAD <> e <> t <> u <> {#e = #u(p);    /*p = tmp1.second;*/}!}
{!MONAD_P <> e <> t <> u <> {#e = &#u(p); /*p = tmp1.second;*/}!}
{!MONAD_F <> e <> t <> u <> {#e(#u(p));   /*p = tmp1.second;*/}!}
{!SKIP <> s <> if (p.tknvals[p.idx] == #s) p.idx++; else cout << "Excepts " << #s << " but " << p.tknvals[p.idx] << endl; !}
{!PARSE <> type <> name <> #type #name(ParserFlow& p) !}

{- PARSE <> Literal <> literal -}
{
    Literal l;
    l.val = NOW;
    p.idx++;
    
    return l;
}

// O - O - O -> O, (O), (O - O -> O)
{- PARSE <> TypeSignature <> type_signature -}
{
    TypeSignature typesig;
    if (NOW == "(") {
        {- SKIP <> "(" -}
    }

    while (NOW != ">") {
        if (NOW == "(") {
            TypeSignature tmp;
            {- MONAD <> tmp <> TypeSignature <> type_signature -}
            typesig.from.push_back(make_shared<TypeSignature>(tmp));
        } else {
            typesig.from.push_back(NOW);
            p.idx++;
        }

        // NOTE: ->があるかどうかでココの内容をfromに入れてよいかが決まるが、それは今はわからないので
        // NOW が)だったり、とにかく-でない場合はそこで終了、関数型でないとみなしto = fromする
        if (NOW != "-") {
            if (NOW == ")") {
                {- SKIP <> ")" -}
            }
            typesig = TypeSignature(typesig.from);
            typesig.from = {};
            return typesig;
        }

        {- SKIP <> "-" -}
    }

    {- SKIP <> ">" -}

    if (NOW == "(") {
        TypeSignature tmp;
        {- MONAD <> tmp <> TypeSignature <> type_signature -}
        typesig.to = make_shared<TypeSignature>(tmp);
    } else {
        typesig.to = NOW;
        p.idx++;
    }

    if (NOW == ")") {
        {- SKIP <> ")" -}
        return typesig;
    }


    return typesig;
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
                l->argarities.emplace_back(0);
                l->argqualities.emplace_back(ArgQuality {});

                if (NEXT == ":") {
                    p.idx++;
                    {- SKIP <> ":" -}
                    TypeSignature tmp;
                    {- MONAD <> tmp <> TypeSignature <> type_signature -}
                    l->argtypes.push_back(tmp);
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
        if (NOW == "(") {
            {- MONAD <> c->l <> shared_ptr<Lambda> <> lambda -}
        } else {
            {- MONAD <> c->lit <> Literal <> literal -}
        }

        while (p.idx < p.tknvals.size()) {
            if (NOW == ")" || NOW == "") {
                break;
            }

            {- MONAD_F <> c->args.emplace_back <> shared_ptr<Code> <> argument -}
        }
    }


//    {- SKIP <> ")" -}
    c->src.end = next(p.tknvals.begin(), p.idx);

    return c;
}
