/* ========================================================================
   $File: parse.cpp $
   $Date: Aug 08 2019 $
   $Revision: $
   $Creator: Creative GP $
   $Notice: (C) Copyright 2019 by Creative GP. All Rights Reserved. $
   ======================================================================== */

#include "emelio.h"

#define NOW p.tknvals[p.idx]
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

int lctn_idx = 0;
array<Lambda, 10000> lctn;


{- PARSE <> Literal <> literal -}
{
    Literal l;
    l.val = p.tknvals[p.idx];
    p.idx++;
    
    return l;
}

{- PARSE <> Lambda* <> lambda -}
{
    Lambda l;

    {- SKIP <> "(" -}

    // 引数
    if (NOW == "|") {
        {- SKIP <> "|" -}

        for (; p.idx < p.tknvals.size(); p.idx++) {
            if (NOW == "|") {
                {- SKIP <> "|" -}
                break;
            } else {
                l.argnames.push_back(NOW);
            }
        }
    }

    {- MONAD <> l.body <> Code <> code -}

    {- SKIP <> ")" -}

    lctn[lctn_idx] = l;
    lctn_idx++;
    
    return &lctn[lctn_idx-1];
}

{- PARSE <> Code <> argument -}
{
    Code c = {};

//    {- SKIP <> "(" -}

    if (NOW == "(") {
        {- MONAD <> c.l <> Lambda* <> lambda -}
    } else {
        {- MONAD <> c.lit <> Literal <> literal -}
    }

//    {- SKIP <> ")" -}

    return c;
}

// ( add 2 3 )
// ( (|x y| x + y) 2 3 )
{- PARSE <> Code <> code -}
{
    Code c = {};

//    {- SKIP <> "(" -}

    if (NOW == "(") {
        {- MONAD <> c.l <> Lambda* <> lambda -}
    } else {
        {- MONAD <> c.lit <> Literal <> literal -}
    }

    while (p.idx < p.tknvals.size()) {
        if (NOW == ")" || NOW == "") {
            break;
        }

        {- MONAD_F <> c.args.push_back <> Code <> argument -}
    }


//    {- SKIP <> ")" -}

    return c;
}
