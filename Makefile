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
	make -C ./AB_Open_MP3
	make -C ./AB_Open_MP4
	make -C ./AB_MPI_Pur
	make -C ./AB_MPI_Open_MP
	
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
	cd AB_Open_MP3/ && make exec
	cd AB_Open_MP4/ && make exec
	cd AB_MPI_Pur/ && make exec
	cd AB_MPI_Open_MP/ && make exec
	
host:
	cp hostfile Open_MP_MPI/hostfile
	cp hostfile MPI_Pur/hostfile
	cp hostfile MPI_Pur2/hostfile
	cp hostfile MPI2_Open_MP/hostfile
	cp hostfile Open_MP_TASK_MPI/hostfile
	cp hostfile AB_MPI_Pur/hostfile
	cp hostfile AB_MPI_Open_MP/hostfile

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
	cd AB_Open_MP3/ && make clean
	cd AB_Open_MP4/ && make clean
	cd AB_MPI_Pur/ && make clean
	cd AB_MPI_Open_MP/ && make clean
