#!/bin/zsh

TIMEFMT='%J   %U  user %S system %P cpu %*E total'$'\n'\
'avg shared (code):         %X KB'$'\n'\
'avg unshared (data/stack): %D KB'$'\n'\
'total (sum):               %K KB'$'\n'\
'max memory:                %M MB'$'\n'\
'page faults from disk:     %F'$'\n'\
'other page faults:         %R'


line_count=`cat testcases.em | wc -l`

if [ "$1" = "v" ] ; then
    echo "wwwwwww TESTING wwwwwwww"
fi


for i in $(seq 0 `expr $line_count / 4`)
do
    lineI1=`expr $i \* 4 + 1`
    lineI2=`expr $i \* 4 + 2`
    lineI3=`expr $i \* 4 + 3`
    lineI4=`expr $i \* 4 + 4`

    title=`sed -n ${lineI1}p testcases.em`
    code=`sed -n ${lineI2}p testcases.em`
    ans=`sed -n ${lineI3}p testcases.em`
    blank=`sed -n ${lineI4}p testcases.em`

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

