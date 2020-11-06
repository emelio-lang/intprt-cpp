#!/bin/zsh

TIMEFMT='%J   %U  user %S system %P cpu %*E total'$'\n'\
'avg shared (code):         %X KB'$'\n'\
'avg unshared (data/stack): %D KB'$'\n'\
'total (sum):               %K KB'$'\n'\
'max memory:                %M MB'$'\n'\
'page faults from disk:     %F'$'\n'\
'other page faults:         %R'

TESTCASES='testcases/testcases.em'
TESTCASES_DIRECTORY='testcases'

line_count=`cat $TESTCASES | wc -l | sed -e 's/ //g'`

if [ "$1" = "v" ] ; then
    echo "wwwwwww TESTING wwwwwwww"
fi

# one liner

for i in $(seq 0 `expr $line_count / 4`)
do
    lineI1=`expr $i \* 4 + 1`
    lineI2=`expr $i \* 4 + 2`
    lineI3=`expr $i \* 4 + 3`
    lineI4=`expr $i \* 4 + 4`

    title=`sed -n ${lineI1}p $TESTCASES`
    code=`sed -n ${lineI2}p $TESTCASES`
    ans=`sed -n ${lineI3}p $TESTCASES`
    blank=`sed -n ${lineI4}p $TESTCASES`

    if [ "$1" = "v" ] ; then
        echo $title
        echo $code
    else
        echo "Testing $code --- "
    fi

    res=`./emelio "$code"`
    if [ "$res" = "$ans" ] ; then
        echo -e "\e[1;32mPassed.\e[0m"

        if [ "$1" = "v" ] ; then
            time ./emelio "$code"
            echo ""
        fi
    else
        echo -e "\e[1;31mError!\e[0m "
        echo "Expected '$ans', but '$res'.";
        echo $title
    fi
done

# file

for file in $TESTCASES_DIRECTORY/*
do
    if [ `basename $file` = "testcases.em" ] ; then
        continue
    fi
    
    title=`sed -n 1p ${file}`
    ans=`sed -n 2p ${file}`
    code=`sed -n '3,$p' ${file}`


    if [ "$1" = "v" ] ; then
        echo "$title ************************************************************"
        echo "$code"
        echo "********************************************************************"
    else
        echo $title
    fi

    res=`./emelio a a a <<< $code`
    if [ "$res" = "$ans" ] ; then
        echo -e "\e[1;32mPassed.\e[0m"

        if [ "$1" = "v" ] ; then
            time ./emelio a a <<< $code
            echo ""
        fi
    else
        echo -e "\e[1;31mError!\e[0m "
        echo "Expected '$ans', but '$res'.";
        echo $title
        echo $code
    fi
        
done

