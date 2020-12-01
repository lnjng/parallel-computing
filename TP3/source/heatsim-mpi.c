#include <assert.h>
#include <stddef.h>

#include "heatsim.h"
#include "log.h"


typedef struct grid_params
{
    int width;
    int height; 
    int padding[2];
}grid_params_t;


int heatsim_init(heatsim_t* heatsim, unsigned int dim_x, unsigned int dim_y) {
    /*
     * TODO: Initialiser tous les membres de la structure `heatsim`.
     *       Le communicateur doit être périodique. Le communicateur
     *       cartésien est périodique en X et Y.
     */
    int dims[] = {dim_x, dim_y};
    int periodic[] = {1,1};
    MPI_Cart_create(MPI_COMM_WORLD,
                    2,
                    dims,
                    periodic,
                    0,
                    &heatsim->communicator);

    MPI_Comm_rank(heatsim->communicator, &heatsim->rank);
    MPI_Cart_coords(heatsim->communicator,heatsim->rank,2, heatsim->coordinates);
    MPI_Cart_shift(heatsim->communicator,0,1,&heatsim->rank_north_peer, &heatsim->rank_south_peer);
    MPI_Cart_shift(heatsim->communicator,1,1,&heatsim->rank_east_peer, &heatsim->rank_west_peer);
    MPI_Comm_size(heatsim->communicator, &heatsim->rank_count);

fail_exit:
    return -1;
}

int heatsim_send_grids(heatsim_t* heatsim, cart2d_t* cart) {
    /*
     * TODO: Envoyer toutes les `grid` aux autres rangs. Cette fonction
     *       est appelé pour le rang 0. Par exemple, si le rang 3 est à la
     *       coordonnée cartésienne (0, 2), alors on y envoit le `grid`
     *       à la position (0, 2) dans `cart`.
     *
     *       Il est recommandé d'envoyer les paramètres `width`, `height`
     *       et `padding` avant les données. De cette manière, le receveur
     *       peut allouer la structure avec `grid_create` directement.
     *
     *       Utilisez `cart2d_get_grid` pour obtenir la `grid` à une coordonnée.
     */

    if (heatsim->rank == 0)
    {
        const int nb_params = 3;
        int gridParamsLength[nb_params] = {1,1,2};
        MPI_Datatype grid_params_type[nb_params] = {MPI_INT,MPI_INT,MPI_INT};
        MPI_Datatype mpi_grid_params_type;
        MPI_Aint grid_params_positions[nb_params];
        grid_params_postions[0] = offsetof(grid_params_t, width);
        grid_params_postions[1] = offsetof(grid_params_t, height);
        grid_params_postions[2] = offsetof(grid_params_t, padding);

        MPI_Type_struct(nb_params,
                        gridParamsLength,
                        grid_params_positions,
                        grid_params_type,
                        &mpi_grid_params_type);
        
        MPI_Type_commit(&mpi_grid_params_type);

        grid_params_t params_to_send;
        params_to_send.width = cart->total_width;
        params_to_send.height = cart->total_height;
        params_to_send.padding[0] = cart->x_offsets;
        params_to_send.padding[1] = cart->y_offsets;


        // sending parameters
        MPI_Isend(&params_to_send,1,mpi_grid_params_type,heatsim->rank_north_peer,heatsim->rank_north_peer,
                heatsim->communicator);
        
        MPI_Isend(&params_to_send,1,mpi_grid_params_type,heatsim->rank_south_peer,heatsim->rank_south_peer,
                heatsim->communicator);

        MPI_Isend(&params_to_send,1,mpi_grid_params_type,heatsim->rank_east_peer,heatsim->rank_east_peer,
                heatsim->communicator);

        MPI_Isend(&params_to_send,1,mpi_grid_params_type,heatsim->rank_west_peer,heatsim->rank_west_peer,
                heatsim->communicator);

        int north_peer_coords[2];
        int south_peer_coords[2];
        int east_peer_coords[2];
        int west_peer_coords[2];

        // coordonnates of neighbors
        MPI_Cart_coords(heatsim->communicator,heatsim->rank_north_peer,2,north_peer_coords);
        MPI_Cart_coords(heatsim->communicator,heatsim->rank_south_peer,2,south_peer_coords);
        MPI_Cart_coords(heatsim->communicator,heatsim->rank_east_peer,2,east_peer_coords);
        MPI_Cart_coords(heatsim->communicator,heatsim->rank_west_peer,2,west_peer_coords);

        // grid to send 
        grid_t* grid_north = cart2d_get_grid(cart, north_peer_coords[0], north_peer_coords[1]);
        grid_t* grid_south = cart2d_get_grid(cart, rank_south_peer[0], rank_south_peer[1]);
        grid_t* grid_east = cart2d_get_grid(cart, rank_east_peer[0], rank_east_peer[1]);
        grid_t* grid_west = cart2d_get_grid(cart, rank_west_peer[0], rank_west_peer[1]);

        //sending grids
        MPI_Type data_type;
        MPI_Type_contiguous(grid->width_padded * grid->height_padded, MPI_DOUBLE, &data_type);

        MPI_Type data_to_send;
        int count[1] = {1};
        MPI_Types type[1] = {data_type};
        MPI_Aint displs[1] ={offsetof(grid_t, data)};

        MPI_Type_struct(1, count, displs, type, &data_to_send)
        MPI_Type_commit(&data_to_send);
        
    }

fail_exit:
    return -1;
}

grid_t* heatsim_receive_grid(heatsim_t* heatsim) {
    /*
     * TODO: Recevoir un `grid ` du rang 0. Il est important de noté que
     *       toutes les `grid` ne sont pas nécessairement de la même
     *       dimension (habituellement ±1 en largeur et hauteur). Utilisez
     *       la fonction `grid_create` pour allouer un `grid`.
     *
     *       Utilisez `grid_create` pour allouer le `grid` à retourner.
     */

fail_exit:
    return NULL;
}

int heatsim_exchange_borders(heatsim_t* heatsim, grid_t* grid) {
    assert(grid->padding == 1);

    /*
     * TODO: Échange les bordures de `grid`, excluant le rembourrage, dans le
     *       rembourrage du voisin de ce rang. Par exemple, soit la `grid`
     *       4x4 suivante,
     *
     *                            +-------------+
     *                            | x x x x x x |
     *                            | x A B C D x |
     *                            | x E F G H x |
     *                            | x I J K L x |
     *                            | x M N O P x |
     *                            | x x x x x x |
     *                            +-------------+
     *
     *       où `x` est le rembourrage (padding = 1). Ce rang devrait envoyer
     *
     *        - la bordure [A B C D] au rang nord,
     *        - la bordure [M N O P] au rang sud,
     *        - la bordure [A E I M] au rang ouest et
     *        - la bordure [D H L P] au rang est.
     *
     *       Ce rang devrait aussi recevoir dans son rembourrage
     *
     *        - la bordure [A B C D] du rang sud,
     *        - la bordure [M N O P] du rang nord,
     *        - la bordure [A E I M] du rang est et
     *        - la bordure [D H L P] du rang ouest.
     *
     *       Après l'échange, le `grid` devrait avoir ces données dans son
     *       rembourrage provenant des voisins:
     *
     *                            +-------------+
     *                            | x m n o p x |
     *                            | d A B C D a |
     *                            | h E F G H e |
     *                            | l I J K L i |
     *                            | p M N O P m |
     *                            | x a b c d x |
     *                            +-------------+
     *
     *       Utilisez `grid_get_cell` pour obtenir un pointeur vers une cellule.
     */
    int nb_transaction = 8;
    MPI_Request req[nb_transaction];
    MPI_Status status[nb_transaction];

    MPI_Datatype vec; 
    MPI_Type_vector(grid->height, 1, grid->width_padded, MPI_DOUBLE, &vec);

    double* left_corner = grid_get_cell(grid, 0,0);
    double* bottom_left_corner = grid_get_cell(grid, 0,grid->height-1);
    double* right_corner = grid_get_cell(grid, 0,grid->height-1);

    // sending
    
    MPI_Isend(left_corner,grid->width,MPI_DOUBLE,heatsim->rank_north_peer,heatsim->rank_north_peer,
                heatsim->communicator, &req[0]);
   
    MPI_Isend(bottom_left_corner ,grid->width,MPI_DOUBLE,heatsim->rank_south_peer,heatsim->rank_south_peer,
                heatsim->communicator, &req[1]);
   
    MPI_Isend(left_corner,1,vec,heatsim->rank_west_peer,heatsim->rank_west_peer,
                heatsim->communicator, &req[2]);

    MPI_Isend(right_corner,1,vec,heatsim->rank_east_peer,heatsim->rank_east_peer,
                heatsim->communicator, &req[3]);

     //receiving

    MPI_Irecv(bottom_left_corner,grid->width,MPI_DOUBLE, heatsim->rank_south_peer,heatsim->rank_south_peer,
         heatsim->communicator, &req[4]);

    MPI_Irecv(left_corner,grid->width,MPI_DOUBLE, heatsim->rank_north_peer,heatsim->rank_north_peer,
         heatsim->communicator, &req[5]);

    MPI_Irecv(right_corner,1,vec,heatsim->rank_east_peer,heatsim->rank_east_peer,
                heatsim->communicator, &req[6]);

    MPI_Irecv(left_corner,1,vec,heatsim->rank_west_peer,heatsim->rank_west_peer,
                heatsim->communicator, &req[7]);
    

    // wait all the transaction
    int ret = MPI_Waitall(nb_transaction,req,nb_transaction);
    if (ret != MPI_SUCCESS)
    {
        goto fail_exit;
    }


fail_exit:
    printf("a problem with one the transactions occured \n");
    return -1;
}

int heatsim_send_result(heatsim_t* heatsim, grid_t* grid) {
    assert(grid->padding == 0);

    /*
     * TODO: Envoyer les données (`data`) du `grid` résultant au rang 0. Le
     *       `grid` n'a aucun rembourage (padding = 0);
     */

fail_exit:
    return -1;
}

int heatsim_receive_results(heatsim_t* heatsim, cart2d_t* cart) {
    /*
     * TODO: Recevoir toutes les `grid` des autres rangs. Aucune `grid`
     *       n'a de rembourage (padding = 0).
     *
     *       Utilisez `cart2d_get_grid` pour obtenir la `grid` à une coordonnée
     *       qui va recevoir le contenue (`data`) d'un autre noeud.
     */

fail_exit:
    return -1;
}
