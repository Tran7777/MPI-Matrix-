#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char **argv) {
	int iEvenStripTag = 1;
	int iOddStripTag = 2;
	int iCurrentRank;
	int iNumberOfPrecesses = 0;
	int iTopBoundaryCondition = 10;
	int iBottomBoundaryCondition = 10;
	int iLeftBoundaryCondition = 10;
	int iRightBoundaryCondition = 10;
	const int iMatrixLength = 10;

	MPI_Status oStatus;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &iNumberOfPrecesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &iCurrentRank);

	int iStripLength = iMatrixLength / iNumberOfPrecesses;

	double dLocalMax;
	double dMaxOfLocalMaxes;
	double dEpsilon = 0.0001;

	double aUCommon[iMatrixLength][iMatrixLength];

	for (int i = 0; i < iMatrixLength; i++) {
		for (int j = 0; j < iMatrixLength; j++) {
			aUCommon[i][j] = 0; //Nach usl
		}
	}

	for (int i = 0; i < iMatrixLength; i++) {
		aUCommon[iMatrixLength - 1][i] = iTopBoundaryCondition; //top
		aUCommon[0][i] = iBottomBoundaryCondition; //bottom
		aUCommon[i][0] = iLeftBoundaryCondition; //left
		aUCommon[i][iMatrixLength - 1] = iRightBoundaryCondition; //rigth
		
	}

	double aULocalOld[iMatrixLength][iMatrixLength];
	double aULocalNew[iMatrixLength][iMatrixLength];

	for (int i = 0; i < iStripLength; i++) {
		for (int j = 0; j < iMatrixLength; j++) {
			aULocalOld[i][j] = aUCommon[iCurrentRank*iStripLength + i][j];
			aULocalNew[i][j] = aULocalOld[i][j];
		}
	}

	double aTempLine[iMatrixLength];

	cout << "hello" << iCurrentRank << endl;
	do {

		for (int i = 0; i < iMatrixLength; i++) {
			aTempLine[i] = aULocalOld[iStripLength - 1][i];
		}

		if (iCurrentRank != iNumberOfPrecesses - 1) {
			MPI_Send(&aTempLine, iMatrixLength, MPI_DOUBLE, iCurrentRank + 1, iEvenStripTag, MPI_COMM_WORLD);
		}
		if (iCurrentRank != 0) {
			MPI_Recv(&aTempLine, iMatrixLength, MPI_DOUBLE, iCurrentRank - 1, iEvenStripTag, MPI_COMM_WORLD, &oStatus);
		}

		for (int i = 0; i < iStripLength; i += 2) {
			for (int j = 1; j < iMatrixLength - 1; j++) {
				
				if (iCurrentRank != 0 && i == 0) {
					aULocalNew[i][j] = 0.25*(aULocalOld[i + 1][j]
						+ aTempLine[j] + aULocalOld[i][j + 1] + aULocalOld[i][j - 1]);
				}
				if (i != 0) {
					aULocalNew[i][j] = 0.25*(aULocalOld[i + 1][j]
						+ aULocalOld[i - 1][j] + aULocalOld[i][j + 1] + aULocalOld[i][j - 1]);
				}
			}
		}

		for (int i = 0; i < iMatrixLength; i++) {
			aTempLine[i] = aULocalNew[0][i];
		}

		if (iCurrentRank != 0) {
			MPI_Send(&aTempLine, iMatrixLength, MPI_DOUBLE, iCurrentRank - 1, iOddStripTag, MPI_COMM_WORLD);
		}
		if (iCurrentRank != iNumberOfPrecesses - 1) {
			MPI_Recv(&aTempLine, iMatrixLength, MPI_DOUBLE, iCurrentRank + 1, iOddStripTag, MPI_COMM_WORLD, &oStatus);
		}

		for (int i = 1; i < iStripLength; i += 2) {
			for (int j = 1; j < iMatrixLength - 1; j++) {
				if (iCurrentRank != iNumberOfPrecesses - 1 && i == iStripLength - 1) {
					aULocalNew[i][j] = 0.25*(aTempLine[j]
						+ aULocalNew[i - 1][j] + aULocalOld[i][j + 1] + aULocalOld[i][j - 1]);
				}
				if (i != iStripLength - 1) {
					aULocalNew[i][j] = 0.25*(aULocalNew[i + 1][j]
						+ aULocalNew[i - 1][j] + aULocalOld[i][j + 1] + aULocalOld[i][j - 1]);
				}
			}
		}

		dLocalMax = fabs(aULocalNew[0][0] - aULocalOld[0][0]);
		for (int i = 0; i < iStripLength; i++) {
			for (int j = 0; j < iMatrixLength; j++) {

				if (fabs(aULocalNew[i][j] - aULocalOld[i][j]) > dLocalMax)
					dLocalMax = fabs(aULocalNew[i][j] - aULocalOld[i][j]);
			}
		}
		for (int i = 0; i < iStripLength; i++) {
			for (int j = 0; j < iMatrixLength; j++) {
				aULocalOld[i][j] = aULocalNew[i][j];
			}
		}

		MPI_Allreduce(&dLocalMax, &dMaxOfLocalMaxes, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

	} while (dMaxOfLocalMaxes > dEpsilon);

	ofstream fout;
	fout.open("file.txt");
	for (int iProcess = 0; iProcess < iNumberOfPrecesses; iProcess++) {
		
		if (iProcess == iCurrentRank) {
			for (int i = 0; i < iStripLength; i++) {
				for (int j = 0; j < iMatrixLength; j++) {
					fout << aULocalNew[i][j] << "\t";
				}
				fout << endl;
			}
		}
		fout.close();
	}

	
	MPI_Finalize();
	return 0;
}


