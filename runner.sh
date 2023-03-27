make clean
make multithreaded single
NO=8
DIREC="testdir"
INT="sex"
for i in {1..1}
do
    echo "TC $i"
    valgrind --leak-check=full --log-file=valgrind.rpt ./multithreaded $NO $DIREC $INT
    #valgrind --tool=helgrind --log-file=valgrind.rpt ./multithreaded $NO $DIREC $INT
    #./multithreaded $NO $DIREC $INT
    #valgrind --leak-check=full --log-file=valgrind.rpt ./single $NO $DIREC $INT
done