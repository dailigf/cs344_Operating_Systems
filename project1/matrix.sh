ls#!/bin/bash

# Program 1 - CS 344
# Francis C. Dailig


trap "rm -f datafile* echo 'Error: Problem Reading stdin, exiting'; exit 1" INT HUP TERM

function dims(){
# Name
#   dims - print number of rows and columsn to stdout
# Synopsis
#   Usage: cat [filename] | ./matrix.sh dims
#          ./matrix.sh dims [filename]
#          ./matrix.sh dims < [filename]
# Description:
#           counts the number of rows and columns of matrix
# Author
#   Written by Franci Dailig (dailigf@oregonstate.edu)

# Variables hold number of rows and columsn
numOfRows=0
numOfCols=0
# isColSet is used so that Column count happens only once.
isColSet=0

while read myLine
do
	(( numOfRows++  ))
	# If statement, so that number of columns is counted only once
	if test $isColSet -eq 0
	then
		(( isColSet++ ))
		for i in $myLine
		do
			(( numOfCols++ ))
		done
	fi
done < $1

echo "$numOfRows $numOfCols"
}

function transpose(){
# Name
#   transpose - reflects elements of the matrix along main diagnol, MxN becomes NxM
# Synopsis
#   Usage: cat [filename] | ./matrix.sh transpose
#	   ./matrix.sh transpose [filename]
#          ./matrix.sh transpose < [filename]
# Description
#	   Reflect elements of that matrix along main diagnol.
# Author
#  Written by Francis Dailig (dailigf@oregonstate.edu)
#

# Variables to get the dimensions of the matrix
line=$(dims $1)
rows=`echo $line | cut -d" " -f 1`
cols=`echo $line | cut -d" " -f 2`
count=0

while read myLine
do
	#using count so that I can create unique files for each row, which will becom ea column
	(( count++ ))
	#touch "tempCol$count"
	for i in $myLine
	do
		# populate column file with elements from the current row
		echo "$i" >> tempCol$count
		#echo $i >> tempCol$$
	done

done < $1

for i in $(eval echo "{1..$cols}")
do
	# thwi will be a string that will be displayed
	tempRow=""
	# number of elements in each column file is the number of orignial row
	for j in $(eval echo "{1..$rows}")
	do
		# tr '\n' to ' ', to make column 'space' delimted, makes it easer it cut each element
		tempVar=$(cat tempCol$j | tr '\n' ' ' | cut -d" " -f $i)

		#if statement to see if we're at the end line, if not, then add a tab
		if [ $j -ne $rows ]
		then
			tempVar=`echo -e $tempVar"\t"`
		fi
		tempRow="${tempRow}$tempVar"
	done

	# output the current row, make it tab delimted
	echo $tempRow | tr ' ' '\t' 
done

#cleanup files
rm tempCol*

}

function add(){
# Name
#	add - function will add two matrices and display output
# Synopsis
#	Usage: ./matrix.sh [matrix_1] [matrix_2]
#
# Description:
#          Will take two MxN matrcies and add them together element wise and produce and MxN matrix
#          Will return an error if the matrices do not have the same dimensions
# Author
#       Written by Francis Dailig (dailigf@oregonstate.edu)
#

#get the dimensions of the two matrices
m1Dim=$(dims $1)
m2Dim=$(dims $2)

#compare the strings to see if they are equal
if [ "$m1Dim" != "$m2Dim" ]
then
	echo "Error: Matrices are not equal." 1>&2
	return 1
fi

#get number of rows and columns, variabled needed to iterate through for loops
line=$(dims $1)
numOfRows=`echo $line | cut -d" " -f 1`
numOfCols=`echo $line | cut -d" " -f 2`

for row in $(eval echo {$numOfRows..1})
do
	# String variable to hold Row elements
	tempRow=""
	for col in $(eval echo {1..$numOfCols})
	do
		# get elements for each matrix, then add
		varM1=$(cat $1 | tail -n $row | head -n 1 | cut -d$'\t' -f $col)
		varM2=$(cat $2 | tail -n $row | head -n 1 | cut -d$'\t' -f $col)
		tempVar=$(( $varM1 + $varM2 ))
		tempRow="${tempRow} ${tempVar}"
	done
	# place $temprow into a temp file
	echo $tempRow >> tempRowFile$$
done
#format temp file, replace spaces with tabs, display contents of file
cat tempRowFile$$ | tr ' ' '\t' >> finalRowFile$$
cat finalRowFile$$

# remove temp files
rm tempRowFile$$
rm finalRowFile$$

}

function mean() {
# Name
#	mean - will take the average of each colum
# Synopsis
#	Usage: ./matrix.sh mean [matrix]
#	       cat [matrix] | ./matrix.sh mean
#
# Description:
#       Takes and MxN matrix and returns a 1XN vectore, where
#       first element is the mean of column one, the second, column two, so and so forth
#
# Author
#       Written by Francis C. Dailig, dailigf@oregonstate.edu
#

# get the dimensions of the matrix
numOfRows=$(dims $1 | cut -d" " -f 1)
numOfCols=$(dims $1 | cut -d" " -f 2)

# transpose the matrix to make it easier to get the mean
# tranposing transforms each column into a row
transpose $1 > m1Transposed$$


tempRow=""

while read myLine
do
	# tempvar will hold the current total of each element added
	tempVar=0
	average=0
	# iterate through each element in the row, which was previously a column
	# it is now a row because it was tranposed
	for i in $(eval echo "{1..$numOfRows}")
	do
		curVar=$(echo $myLine | cut -d" " -f $i)
		tempVar=$(( $tempVar + $curVar ))
	done

	# implementation of the mean formula provided
	if [ $tempVar -gt 0 ]
	then
		average=$(( (tempVar + (numOfRows / 2)) / numOfRows  ))
	else
		average=$(( (tempVar - (numOfRows / 2)) / numOfRows ))
	fi
	#average=$(( (tempVar + (numOfRows / 2)) / numOfRows ))
	tempRow="${tempRow} ${average}"
done < m1Transposed$$

echo $tempRow | tr ' ' '\t' > finalFile$$
cat finalFile$$

#cleanup files
rm m1Transposed*
rm finalFile*

}

function multiply() {
# Name
#	multiply - function will multiply two matrices
# Synopsis
#	Usage: ./matrix.sh [matrix_1] [matrix_2]
#
# Description:
#       Takes an MxN and NxP matrix and procuces an MxP matrix.
#
# Author
#       Written by Francis C. Dailig (dailigf@oregonstate.edu)
#

# get dimensions of each matrix
m1Rows=$(dims $1 | cut -d" " -f 1)
m1Colums=$(dims $1 | cut -d" " -f 2)
m2Rows=$(dims $2 | cut -d" " -f 1)
m2Colums=$(dims $2 | cut -d" " -f 2)


# if columns of matrix 1 are not equal to rows of matrix two, throw an error
if [ "$m1Colums" != "$m2Rows" ]
then
	echo "Error: Matrix 1 Columns is not Equal to Matrix 2 Rows, unable to multiply." 1>&2
	return 1
fi

# get a matrix 2 transposed, will make it easier to get the column
transpose $2 | tac > m2Transposed$$
# reverse the contents of the first matrix, it will make it easier to get the rows using tail and head
tac $1 > m1Reversed$$

for i in $(eval echo "{1..$m1Rows}")
do
	m1Row=$(cat m1Reversed$$ | tail -n $i | head -n 1 | tr '\t' ' ')
	tempRow=""

	for j in $(eval echo "{1..$m2Colums}")
	do
		m2Col=$(cat m2Transposed$$ | tail -n $j | head -n 1 | tr '\t' ' ')
		tempVar=0

		for x in $(eval echo "{1..$m2Rows}")
		do
			m1Var=$(echo $m1Row | cut -d" " -f $x)
			m2Var=$(echo $m2Col | cut -d" " -f $x)
			tempVar=$((tempVar+m1Var*m2Var))
		done

		tempRow="${tempRow} ${tempVar}"
	done
	echo $tempRow | tr ' ' '\t' >> finalFile$$
done

# cleanup files
rm m2Transposed*
rm m1Reversed*
cat finalFile$$
rm finalFile*


}
#set no case match
shopt -s nocasematch

if [[ "$1" == "dims" ]]
then
	# if file passed in by stderr it will cat to a tempfile
	if [ "$#" = "1" ]
	then
		cat > datafile$$
		dims datafile$$
		rm datafile$$
	elif [ "$#" = "2" ]
	then
		# test to see if file exists, if not exit 1
		if [ ! -f $2 ]
		then
			echo "Error: file $2 does not exist" 1>&2
			exit 1
			# test to see if the file is readable, if not exit 1
		elif [ ! -f $2 ]
		then
			echo "Error: file $2 does not exit" >&2
			exit 1
		else
			dims $2
		fi
	else
		echo "Error: In Add function, more than one argument passed" 1>&2
		exit 1
	fi
elif [[ "$1" == "transpose" ]]
then
	# if file passed through by stdin it will cat to a tempfile
	if [ "$#" = "1" ]
	then
		cat > datafile$$
		transpose datafile$$ 
		# remove data file after returning from function
		rm datafile*
	elif [ "$#" = "2" ]
	then
		# test to see if file exists, if not exit 1
		if [ ! -f $2 ]
		then
			echo "Error: file $2 does not exist" >&2
			exit 1
			# test to see if file is readable
		elif [ ! -r $2 ]
		then
			echo "Error: file $2 not readable" >&2
			exit 1
		else

			transpose $2
		fi
	else
		echo "Error: you did not pass the correct number of argument." >&2
		exit 1
	fi
elif [[ "$1" == "mean" ]]
then
	# if file passed through by stdin cat to a tempfile
	if [ "$#" = 1 ]
	then
		cat > datafile$$
		mean datafile$$
		rm datafile$$
	elif [ "$#" = "2" ]
	then
		# test to see if file exist, if not exit 1
		if [ ! -f $2 ]
		then
			echo "Error: file $2 does not exist" >&2
			exit 1
			# test to see if file is readable, if not exit 1
		elif [ ! -r $2 ]
		then
			echo "Error: file $2 is not readable" >&2
			exit 1
		else
			mean $2
		fi
	else
		echo "Error: you did not pass in the correct number of arguments" >&2
		exit 1
	fi
elif [[ "$1" == "add" ]]
then
	if [ "$#" = "3" ]
	then
		add $2 $3
	else
		echo "Error: Add function requires two matrices." 1>&2
		exit 1
	fi
elif [[ "$1" == "multiply" ]]
then
	if [ "$#" = "3" ]
	then
		multiply $2 $3
	else
		echo "Error: multiply function requires two matcies." 1>&2
		return 1
	fi

else
	echo "Error: Valid funciton not provided." 1>&2
	return 1
fi

