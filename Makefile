target:
	make -C ./sequentiel
	make -C ./Open_MP
	make -C ./Open_MP_MPI
	make -C ./Open_MP_TASK
	make -C ./MPI_Pur
	make -C ./MPI_Pur2
	make -C ./MPI2_Open_MP
	make -C ./Open_MP_TASK_MPI
	make -C ./AB_sequentiel
	make -C ./AB_Open_MP
	make -C ./AB_Open_MP2
	
exec:
	cd Open_MP/ && make exec
	cd sequentiel/ && make exec
	cd Open_MP_MPI/ && make exec
	cd Open_MP_TASK/ && make exec
	cd MPI_Pur/ && make exec
	cd MPI_Pur2/ && make exec
	cd MPI2_Open_MP/ && make exec
	cd Open_MP_TASK_MPI/ && make exec
	cd AB_sequentiel/ && make exec
	cd AB_Open_MP/ && make exec
	cd AB_Open_MP2/ && make exec

clean:
	cd Open_MP/ && make clean
	cd sequentiel/ && make clean
	cd Open_MP_MPI/ && make clean
	cd Open_MP_TASK/ && make clean
	cd MPI_Pur/ && make clean
	cd MPI_Pur2/ && make clean
	cd MPI2_Open_MP/ && make clean
	cd Open_MP_TASK_MPI/ && make clean
	cd AB_sequentiel/ && make clean
	cd AB_Open_MP/ && make clean
	cd AB_Open_MP2/ && make clean
