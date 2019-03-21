#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>

using namespace std;


int main(int argc, char **argv) {


	//finB.open("A.txt");

		/*for (int i = 0; i < iMatrixSize; i++) {
			for (int j = 0; j < iMatrixSize; j++) {
				fin >> A[i][j];
				//finB >> B[i][j];
			}
		}
		fin.close();*/
	int iCurrentRank;
	int iNumberOfPrecesses;
	const int MASTER_PROCESS = 1;
	const int iMatrixLength = 6;
	const int INIT_NUMBER = 0;

	MPI_Status oStatus;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &iNumberOfPrecesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &iCurrentRank);

	int iStripLength = iMatrixLength / (iNumberOfPrecesses);

	int A[iMatrixLength][iMatrixLength] =
	{ {1, 2, 3, 4, 5, 6},
	{1, 2, 3, 4, 5, 6},
	{1, 2, 3, 4, 5, 6},
	{1, 2, 3, 4, 5, 6},
	{1, 2, 3, 4, 5, 6} ,
	{1, 2, 3, 4, 5, 6} };

	int B[iMatrixLength][iMatrixLength] =
	{ {1, 2, 3, 4, 5, 6},
		{1, 2, 3, 4, 5, 6},
		{1, 2, 3, 4, 5, 6},
		{1, 2, 3, 4, 5, 6},
		{1, 2, 3, 4, 5, 6},
		{1, 2, 3, 4, 5, 6} };

	int LocalA[iMatrixLength][iMatrixLength];
	int LocalB1[iMatrixLength][iMatrixLength];
	int LocalU[iMatrixLength][iMatrixLength];
	int LocalC[iMatrixLength][iMatrixLength];

	for (int i = 0; i < iStripLength; i++) {
		for (int j = 0; j < iMatrixLength; j++) {
			LocalA[i][j] = A[iCurrentRank * iStripLength + i][j];
		}
	}

	for (int i = 0; i < iMatrixLength; i++) {
		for (int j = 0; j < iStripLength; j++) {
			LocalB1[i][j] = B[i][iCurrentRank * iStripLength + j];
		}
	}

	for (int iIterationNumber = 0; iIterationNumber < iNumberOfPrecesses; iIterationNumber++) {

		int C[iMatrixLength][iMatrixLength];

		if (iIterationNumber != INIT_NUMBER) {
			int iPreviousProcess = iCurrentRank + 1 > iNumberOfPrecesses - 1 ? 0 : iCurrentRank + 1;
			MPI_Recv(&LocalB1, iMatrixLength*iMatrixLength, MPI_INT, iPreviousProcess, iIterationNumber, MPI_COMM_WORLD, &oStatus);
		}

		for (int i = 0; i < iMatrixLength; i++) {
			for (int j = 0; j < iMatrixLength; j++) {
				C[i][j] = 0;
			}

		}

		for (int k = 0; k < iStripLength; k++) {
			for (int i = 0; i < iStripLength; i++) {
				C[k][i] = 0;
				for (int j = 0; j < iMatrixLength; j++) {
					C[k][i] += LocalA[k][j] * LocalB1[j][i];
					LocalC[k][i] = C[k][i];
				}
			}
		}

		for (int i = 0; i < iStripLength; i++) {
			for (int j = 0; j < iStripLength; j++) {
				LocalU[i][iIterationNumber * iStripLength + j] = LocalC[i][j];
			}
		}

		if (iIterationNumber == iNumberOfPrecesses - 1 && iCurrentRank != MASTER_PROCESS) {

			MPI_Send(LocalU, iMatrixLength*iMatrixLength, MPI_INT, MASTER_PROCESS, 999, MPI_COMM_WORLD);
		}
		else if (iIterationNumber < iNumberOfPrecesses - 1) {
			int iNextProcess = iCurrentRank > 0 ? iCurrentRank - 1 : iNumberOfPrecesses - 1;
			MPI_Send(LocalB1, iMatrixLength*iMatrixLength, MPI_INT, iNextProcess, iIterationNumber + 1, MPI_COMM_WORLD);
		}
	}

	if (iCurrentRank == MASTER_PROCESS) {
		int LocalU3[iMatrixLength][iMatrixLength];
		int FinalC[iMatrixLength][iMatrixLength];
		vector<vector<int>>	vFinalC;

		for (int i = 0; i < iMatrixLength; i++) {
			vector<int>	vFinalCol(iMatrixLength);
			vFinalC.push_back(vFinalCol);
		}

		for (int i = 0; i < iStripLength; i++) {
			for (int j = 0; j < iMatrixLength; j++) {
				vFinalC[MASTER_PROCESS * iStripLength + i][j] = LocalU[i][j];
			}
		}

		for (int i = 0; i < iNumberOfPrecesses; i++) {
			if (i != MASTER_PROCESS) {
				MPI_Recv(LocalU3, iMatrixLength*iMatrixLength, MPI_INT, i, 999, MPI_COMM_WORLD, &oStatus);
				for (int j = 0; j < iStripLength; j++) {
					for (int k = 0; k < iMatrixLength; k++) {
						vFinalC[iStripLength * i + j][k] = LocalU3[j][k];
					}
				}
			}

		}

		for (int i = 0; i < iNumberOfPrecesses; i++) {
			for (int j = 0; j < iStripLength; j++) {
				for (int k = 0; k < i * iStripLength; k++) {
					vFinalC[iStripLength * i + j].emplace(
						vFinalC[iStripLength * i + j].begin(),
						vFinalC[iStripLength * i + j][iMatrixLength - 1]);
				}

			}
		}

		for (int i = 0; i < iMatrixLength; i++) {
			for (int j = 0; j < iMatrixLength; j++) {
				cout << vFinalC[i][j] << "  ";
			}
			cout << "\n\n";
		}

	}

	MPI_Finalize();
	return 0;
}


