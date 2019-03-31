#include <mpi.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <concurrent_vector.h>
#include <algorithm>
#include<ctime>

using namespace std;

int getNeibourSize(int *aNeinour, int originalMassSize) {
	int sizeOfNeibour = 0;
	for (int i = 0; i < originalMassSize; i++) {
		if (aNeinour[i] != -858993460) {
			sizeOfNeibour++;
		}
	}
	return sizeOfNeibour;
}

void prinResult(int iCurrentRank, int *recBuf, int iMatrixLength) {
	if (iCurrentRank == 0) {
		cout << iCurrentRank << " process has: ";
		for (int i = 0; i < iMatrixLength; i++) {
			cout << recBuf[i] << "  ";
		}
	}
}

struct smallers {
	bool operator()(const long& a, const long& b) const {
		return a < b;
	}
};

vector<int> oddEvenSort(int iCurrentRank, vector<int> Local_Vector, int iMatrixLength, bool Parity) {
	MPI_Status oStatus;
	int to;
	if (Parity) {
		to = iCurrentRank % 2 == 0 ? iCurrentRank + 1 : iCurrentRank - 1;
	}
	else {
		to = iCurrentRank % 2 != 0 ? iCurrentRank + 1 : iCurrentRank - 1;
	}

	vector <int> NeibourLocalVector(iMatrixLength);
	fill(NeibourLocalVector.begin(), NeibourLocalVector.end(), -858993460);

	MPI_Sendrecv(Local_Vector.data(), Local_Vector.size(), MPI_INT, to, 6, NeibourLocalVector.data(), iMatrixLength, MPI_INT, to, 6, MPI_COMM_WORLD, &oStatus);

	int sizeOfNeibour = getNeibourSize(NeibourLocalVector.data(), iMatrixLength);
	NeibourLocalVector.resize(sizeOfNeibour);

	int iLocalFullVectorSize = Local_Vector.size() + NeibourLocalVector.size();
	vector <int> LocalFullVector(iLocalFullVectorSize);

	merge(begin(Local_Vector), end(Local_Vector), begin(NeibourLocalVector), end(NeibourLocalVector), begin(LocalFullVector));

	int start;
	if (Parity) {
		start = iCurrentRank % 2 == 0 ? 0 : Local_Vector.size();
	}
	else {
		start = iCurrentRank % 2 == 1 ? 0 : sizeOfNeibour;
	}

	for (int i = 0; i < Local_Vector.size(); i++) {
		Local_Vector[i] = LocalFullVector[start + i];
	}
	return Local_Vector;
}

vector<int> sortArray(int iCurrentRank , vector<int>Local_Vector, int iNumberOfPrecesses, int iMatrixLength) {
	int u = 1;
	while (u < 16) {
		if (u % 2 == 1) {
			Local_Vector = oddEvenSort(iCurrentRank, Local_Vector, iMatrixLength, true);
		}
		else if (u % 2 == 0) {
			if (iCurrentRank != 0 && iCurrentRank < iNumberOfPrecesses - 1) {
				Local_Vector = oddEvenSort(iCurrentRank, Local_Vector, iMatrixLength, false);
			}
		}
		u++;
	}
	return Local_Vector;
}

int main(int argc, char **argv) {
	int iCurrentRank;
	int iNumberOfPrecesses;
	const int iMatrixLength = 20;

	MPI_Status oStatus;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &iNumberOfPrecesses);
	MPI_Comm_rank(MPI_COMM_WORLD, &iCurrentRank);

	int rem = (iMatrixLength) % iNumberOfPrecesses; // elements remaining after division among processes
	int sum = 0;                // Sum of counts. Used to calculate displacements
	  // buffer where the received data should be stored
	int *sendcounts = new int[iNumberOfPrecesses];
	int *displs = new int[iNumberOfPrecesses];
	// calculate send counts and displacements
	for (int i = 0; i < iNumberOfPrecesses; i++) {
		sendcounts[i] = (iMatrixLength) / iNumberOfPrecesses;
		if (rem > 0) {
			sendcounts[i]++;
			rem--;
		}

		displs[i] = sum;
		sum += sendcounts[i];
	}

	int localVectorSize = sendcounts[iCurrentRank];
	vector<int> Local_Vector(localVectorSize);

	vector<int> Orig_Vector = { 17 ,16, 15,14 ,13,12,11,10,9,8,7,6,5,4,3,2,1,0, -1, -2 };
	MPI_Scatterv(Orig_Vector.data(), sendcounts, displs, MPI_INT, Local_Vector.data(), 100, MPI_INT, 0, MPI_COMM_WORLD);

	sort(begin(Local_Vector), end(Local_Vector));
	vector <int> VPrevStateVector(Local_Vector.size());
	vector <int> recBuf(iMatrixLength);

	vector <int> vFinalLocalVector(Local_Vector.size());

	vFinalLocalVector = sortArray(iCurrentRank,Local_Vector,iNumberOfPrecesses,iMatrixLength);
	MPI_Gatherv(vFinalLocalVector.data(), Local_Vector.size(), MPI_INT, recBuf.data(), sendcounts, displs,MPI_INT,0,MPI_COMM_WORLD);
	prinResult(iCurrentRank, recBuf.data(), iMatrixLength);

	MPI_Finalize();
	return 0;
}


