#include <assert.h>
#include <stddef.h>

#include "heatsim.h"
#include "log.h"

int init_struct_whp(MPI_Datatype* whp_params){
    int nb_params = 3;
    int blocklengths[nb_params];
    MPI_int indices[nb_params];
    MPI_Datatype old_types[nb_params];
    blocklengths[0] = 1;
    blocklengths[1] = 1;
    blocklengths[2] = 1;
    old_types[0] = MPI_INTEGER;
    old_types[1] = MPI_INTEGER;
    old_types[2] = MPI_INTEGER;
    indices[0] = offsetof(grid_t, width);
    indices[1] = offsetof(grid_t, height);
    indices[2] = offsetof(grid_t, padding);

    return MPI_Type_struct(nb_params, blocklengths, indices, old_types, &whp_params);
}

int init_struct_data(MPI_Datatype* data_params, grid_t* grid) {
    int len_dyn_array = grid->width_padded*grid->height_padded;
    MPI_Datatype dyn_array_type;
    int err = MPI_Type_contiguous(len_dyn_array, MPI_DOUBLE, &dyn_array_type);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("Failed MPI_Type_contiguous", err);
        return err;
    }

    int nb_params = 1;
    int blocklengths[nb_params];
    MPI_int indices[nb_params];
    MPI_Datatype old_types[nb_params];
    blocklengths[0] = 1;
    old_types[0] = dyn_array_type;
    indices[0] = offsetof(grid_t, data);
    return MPI_Type_struct(nb_params, blocklengths, indices, old_types, &data_params);
}

int heatsim_init(heatsim_t* heatsim, unsigned int dim_x, unsigned int dim_y) {
    /*
     * TODO: Initialiser tous les membres de la structure `heatsim`.
     *       Le communicateur doit être périodique. Le communicateur
     *       cartésien est périodique en X et Y.
     */
    int err = MPI_SUCCESS;

    err = MPI_Comm_size(MPI_COMM_WORLD, &heatsim->rank_count);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_init - Failed MPI_Comm_size", err);
        goto fail_exit;
    }
    err = MPI_Comm_rank(MPI_COMM_WORLD, &heatsim->rank);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_init - Failed MPI_Comm_rank", err);
        goto fail_exit;
    }

	if (dim_x * dim_y != heatsim->rank_count) {
		printf("heatsim_init - Block size needs to be the same value as nb of rank count");
		goto fail_exit;
	}

    err = MPI_Cart_create(MPI_COMM_WORLD, 2, [dim_x, dim_y], [1, 1], false, &heatsim->communicator);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_init - Failed MPI_Cart_create", err);
        goto fail_exit;
    }
    err = MPI_Cart_shift(heatsim->communicator, 1, 1, &heatsim->rank_north_peer, &heatsim->rank_south_peer);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_init - Failed MPI_Cart_shift (1,1)", err);
        goto fail_exit;
    }
    err = MPI_Cart_shift(heatsim->communicator, 0, 1, &heatsim->rank_west_peer, &heatsim->rank_east_peer);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_init - Failed MPI_Cart_shift (0,1)", err);
        goto fail_exit;
    }
    err = MPI_Cart_coords(MPI_COMM_WORLD, heatsim->rank, 2, heatsim->coordinates);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_init - Failed MPI_Cart_coords", err);
        goto fail_exit;
    }

    return 1;

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

    int nb_msg_send = 2;

    int nb_requests = (rank_count-1)*nb_msg_send;

    //better to have pointers?
    MPI_Request reqs[nb_requests*nb_msg_send];
    MPI_Status stats[nb_requests*nb_msg_send];

    for (int i = 1; i < rank_count; i++) {
        int err = MPI_SUCCESS;
        int tag = 0;

        int coordinates[2]; 
        err = MPI_Cart_coords(heatsim->communicator, i, 2, coordinates);

        if (err != MPI_SUCCESS){
            LOG_ERROR_MPI("heatsim_send_grids- Failed MPI_Cart_coords", err);
            goto fail_exit;
        }
        grid_t* grid = cart2d_get_grid(cart, coordinates[0], coordinates[1]);

        MPI_Datatype whp_params;
        MPI_Datatype data_params;
        err = init_struct_whp(&whp_params);
        if (err != MPI_SUCCESS){
            LOG_ERROR_MPI("heatsim_send_grids - Failed init_struct_whp", err);
            goto fail_exit;
        }
        err = init_struct_data(&data_params, grid);
        if (err != MPI_SUCCESS){
            LOG_ERROR_MPI("heatsim_send_grids - Failed init_struct_data", err);
            goto fail_exit;
        }
        err = MPI_Type_commit(&whp_params);
        if (err != MPI_SUCCESS){
            LOG_ERROR_MPI("heatsim_send_grids - Failed MPI_Type_commit (whp_params)", err);
            goto fail_exit;
        }
        err = MPI_Type_commit(&data_params);
        if (err != MPI_SUCCESS){
            LOG_ERROR_MPI("heatsim_send_grids - Failed MPI_Type_commit (data_params)", err);
            goto fail_exit;
        }

        err = MPI_Isend(grid, 1, whp_params, i, tag, heatsim->communicator, &reqs[(i-1)*nb_msg_send]);
        if (err != MPI_SUCCESS){
            LOG_ERROR_MPI("heatsim_send_grids - Failed MPI_Isend (data_params)", err);
            goto fail_exit;
        }
        err = MPI_Isend(grid, 1, data_params, i, tag, heatsim->communicator, &reqs[(i-1)*nb_msg_send+1]);
        if (err != MPI_SUCCESS){
            LOG_ERROR_MPI("heatsim_send_grids - Failed MPI_Isend (data_params)", err);
            goto fail_exit;
        }
    }

    err = MPI_Waitall(nb_requests, reqs, stats);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_send_grids - Failed MPI_Waitall", err);
        goto fail_exit;
    }

    err = MPI_Type_free(&whp_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_send_grids - Failed MPI_Type_free (whp_params)", err);
        goto fail_exit;
    }
    err = MPI_Type_free(&data_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_send_grids - Failed MPI_Type_free (data_params)", err);
        goto fail_exit;
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
    int err = MPI_SUCCESS;
    int tag = 0;
    
    MPI_Request reqwhp;
    MPI_Status statwhp;

    MPI_Datatype whp_params;
    err = init_struct_whp(&whp_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed init_struct_whp", err);
        goto fail_exit;
    }
    err = MPI_Type_commit(&whp_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Type_commit(whp_params)", err);
        goto fail_exit;
    }

    grid_t g;
    err = MPI_Irecv(&g, 1, whp_params, 0, tag, heatsim->communicator, &reqwhp);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Irecv(whp_params)", err);
        goto fail_exit;
    }
    err = MPI_Wait(&reqwhp, &statwhp);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Wait(whp_params)", err);
        goto fail_exit;
    }

    err = MPI_Type_free(&whp_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Type_free(whp_params)", err);
        goto fail_exit;
    }

    grid_t* grid = grid_create(g.width, g.height, g.padding);

    MPI_Datatype data_params;
    err = init_struct_data(&data_params, grid);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed init_struct_data(data_params)", err);
        goto fail_exit;
    }
    err = MPI_Type_commit(&data_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Type_commit(data_params)", err);
        goto fail_exit;
    }

    MPI_Request req;
    MPI_Status stat;
    err = MPI_Irecv(grid, 1, data_params, 0, tag, heatsim->communicator, &req);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Irecv(data_params)", err);
        goto fail_exit;
    }
    err = MPI_Wait(&req, &stat);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Wait(data_params)", err);
        goto fail_exit;
    }
    err = MPI_Type_free(&data_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_receive_grid - Failed MPI_Type_free(data_params)", err);
        goto fail_exit;
    }

    return grid;

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

fail_exit:
    return -1;
}

int heatsim_send_result(heatsim_t* heatsim, grid_t* grid) {
    assert(grid->padding == 0);

    /*
     * TODO: Envoyer les données (`data`) du `grid` résultant au rang 0. Le
     *       `grid` n'a aucun rembourage (padding = 0);
     */
    MPI_Datatype data_params;
    init_struct_data(&data_params, grid);
    MPI_Type_commit(&data_params);

    MPI_Request req;
    MPI_Status stat;
    int len_dyn_array = grid->height_padded*grid->width_padded;

    err |= MPI_Isend(grid, 1, data_params, 0, tag, heatsim->communicator, &req);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_send_result - Failed MPI_Isend", err);
        goto fail_exit;
    }
    err |= MPI_Wait(&req, &stat);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_send_result - Failed MPI_Wait", err);
        goto fail_exit;
    }
    err = MPI_Type_free(&data_params);
    if (err != MPI_SUCCESS){
        LOG_ERROR_MPI("heatsim_send_result - Failed MPI_Type_free", err);
        goto fail_exit;
    }

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
