/*Code written by David Mitchell based on instruction from Prof Wendy Myrvold University of Victoria */


#include <stdlib.h>
#include <stdio.h>

#define NMAX 256

struct Graph
{
    int adj_list[NMAX][NMAX];
    int edge_list[NMAX][NMAX];
    int num_nodes;
    int num_edges[NMAX];
    int graph_number;
    int total_edges;
    int base_graph_number;
};

int read_graph(struct Graph *graph, FILE * file);
void find_cf(struct Graph *graph);
void flip_graph(struct Graph *graph, struct Graph *flip);
void cw_bfs(int r, int j, struct Graph * graph, struct Graph * curr_cf);
int find_pp (int vert, int edge_number, struct Graph *graph);
int cmp_cf (struct Graph *cf1, struct Graph *cf2);
void copy_cf(struct Graph *min_cf, struct Graph *curr_cf);
void print_graph(struct Graph *graph);
void order_graph(struct Graph *graph);
void rotate_row(int vert, int shift, struct Graph *graph);

/**********************************************************************************/

    
int main(int argc, char * argv[])
{
    
    int graph_number = 1;
    //clear the output streams;
    fflush(stdout);
    
    //open the file containing the graph
    FILE *file;
    if (argc > 1)
    {
        file = fopen(argv[1],"r");
        //fprintf(stderr,"Please use stdin\n");
        //exit(0);
    }
    else
    {
        file = stdin;
    }
    
    //read the graphs
    struct Graph graph;

    while (read_graph(&graph, file))
    {
        graph.graph_number = graph_number;
        find_cf(&graph);
        graph_number++;
    }
}


/**********************************************************************************/

int read_graph(struct Graph *graph,FILE * file)
{
    int i;
    int j;
    
    //read the base graph number
    if (!fscanf(file,"%d",&graph->base_graph_number))
    {
        fprintf(stderr,"error reading in graph (error using scanf)\n");
        exit(1);
    }
    
    //if we've reached the end of the file return 0
    if (feof(file))
    {
        return 0;
    }
    
    //read the total number of nodes
    if (!fscanf(file,"%d",&graph->num_nodes))
    {
        fprintf(stderr,"error reading in graph (error using scanf)\n");
        exit(1);
    }
    
    //check the total number of nodes
    if (graph->num_nodes < 1) {
        fprintf(stderr, "error reading in graph\n");
        exit(1);
    }
    
    graph->total_edges = 0;
    
    //for each row use scan f to read the # of edges and insert them in matrix
    for (i = 0; i < graph->num_nodes; i++)
    {
        if (!fscanf(file, "%d", &graph->num_edges[i]))
        {
            fprintf(stderr, "error reading in graph (error using scanf)\n");
            exit(1);
        }
        
        if (graph->num_edges[i] < 0 || graph->num_edges[i] > graph->num_nodes)
        {
            fprintf(stderr, "error reading in graph (incorrect number of edges)\n");
            exit(1);
        }
        
        graph->total_edges += graph->num_edges[i];
        
        //read nodes into the graph
        for (j = 0; j < graph->num_edges[i]; j++)
        {
            if (!fscanf(file, "%d",&graph->adj_list[i][j]))
            {
                fprintf(stderr, "error reading in graph (error using scanf)\n");
                exit(1);
            }
            
            if (graph->adj_list[i][j] > graph->num_nodes || graph->adj_list[i][j] < 0 || graph->adj_list[i][j] == i)
            {
                fprintf(stderr, "error reading in graph (incorrect node)\n");
                exit(1);
            }
            
            //read the associated edge into the graph
            if (!fscanf(file,"%d",&graph->edge_list[i][j]))
            {
                fprintf(stderr, "error reading in graph (error using scanf)\n");
                exit(1);
            }
            
            graph->edge_list[i][j] = abs(graph->edge_list[i][j])-2;
        }
    }
    graph->total_edges = graph->total_edges/2;
    return 1;
    
}

/**********************************************************************************/


void find_cf(struct Graph *graph)
{
    int r,d,j,first, count;
    count = 1;
    first = 1;
    struct Graph flip;
    struct Graph min_cf;
    struct Graph curr_cf;
    
    //compose a flip graph where the vertices are traversed in the opposite direction
    flip_graph(graph,&flip);
    
    //set the direction as clockwise or counterclockwise
    for (d = 1; d >= -1; d = d-2)
    {
        //iterate over all possible roots
        for (r = 0; r < graph->num_nodes; r++)
        {
            //iterate over all possible starting nodes
            for (j = 0; j < graph->num_edges[r]; j++)
            {
                if (d==1)
                    cw_bfs(r,j,&flip,&curr_cf);
                else
                    cw_bfs(r,j,graph,&curr_cf);

                //order the graph
                order_graph(&curr_cf);
                
                //if this is the first cf, copy it to min
                if (first == 1)
                {
                    copy_cf(&min_cf,&curr_cf);
                    first = 0;
                }
                else
                {
                    if (cmp_cf(&min_cf,&curr_cf) == -1)
                    {
                        copy_cf(&min_cf, &curr_cf);
                    }
                }

            }
        }
    }
    
    print_graph(&min_cf);
}

/**********************************************************************************/


void cw_bfs(int r, int j, struct Graph * graph, struct Graph * curr_cf)
{
    int i, pos, num_nbrs, ug_num_nodes;
    
    ug_num_nodes = (int) ((double) graph->num_nodes*2/5);
    
    //update values of curr cf
    curr_cf->graph_number = graph->graph_number;
    curr_cf->num_nodes = graph->num_nodes;
    curr_cf->total_edges = graph->total_edges;
    
    for (i = 0; i < curr_cf->num_nodes; i++)
    {
        curr_cf->num_edges[i] = 0;
    }
    
    int Q[NMAX];
    int qfront;
    int qrear;
    int parent_position[NMAX];
    int edge_position;
    int BFIv[NMAX];
    
    //nodes which are part of the underlying graph begin at a lower index
    int BFIvl_count;
    int BFIvh_count;
    
    int BFIe[NMAX];
    int BFIe_count;
    
    int vst_vert;
    int nbr_vert;
    int nbr_edge;
    
    //set BFIv to -1
    for (i = 0; i < graph->num_nodes; i++)
    {
        BFIv[i] = -1;
    }
    
    BFIvl_count = 0;
    BFIvh_count = ug_num_nodes;
    
    //set BFIe to -1
    for (i = 0; i < graph->total_edges; i++)
    {
        BFIe[i] = -1;
    }
    BFIe_count = 0;
    qfront = 0;
    qrear = 1;
    Q[0] = r;
    
    if (r < ug_num_nodes)
    {
        BFIv[r] = BFIvl_count;
        BFIvl_count++;
    }
    else
    {
        BFIv[r] = BFIvh_count;
        BFIvh_count++;
    }
    
    parent_position[r] = j;
    
    while (qfront < qrear)
    {
        //the vertex we are currently visiting is at the head of the queue
        vst_vert = Q[qfront];
        qfront++;
        num_nbrs = graph->num_edges[vst_vert];
        pos = parent_position[vst_vert];
        
        for (i = 0; i < num_nbrs; i++)
        {
            nbr_vert = graph->adj_list[vst_vert][(pos+i)%num_nbrs];
            nbr_edge = graph->edge_list[vst_vert][(pos+i)%num_nbrs];
            edge_position = find_pp(nbr_vert,nbr_edge,graph);
            
            //determine if the nbr vert has been visited
            if (BFIv[nbr_vert] == -1)
            {
                //it hasn't. label it in the BFI
                if (nbr_vert < ug_num_nodes)
                {
                    BFIv[nbr_vert] = BFIvl_count;
                    BFIvl_count++;
                }
                else
                {
                    BFIv[nbr_vert] = BFIvh_count;
                    BFIvh_count++;
                }
                
                
                //add it to the queue
                Q[qrear] = nbr_vert;
                qrear++;
                parent_position[nbr_vert] = edge_position;
            }
            
            //determine if the nbr edge has been visited
            if (BFIe[nbr_edge] == -1)
            {
                //it hasn't. label it in BFI
                BFIe[nbr_edge] = BFIe_count;
                BFIe_count++;
                
                //increase the number of edges of the visiting and neighbour vertex
                curr_cf->num_edges[BFIv[vst_vert]]++;
                curr_cf->num_edges[BFIv[nbr_vert]]++;
                
                //add it to the curr CF
                curr_cf->adj_list[BFIv[vst_vert]][(pos+i)%num_nbrs] = BFIv[nbr_vert];
                curr_cf->edge_list[BFIv[vst_vert]][(pos+i)%num_nbrs] = BFIe[nbr_edge];
                curr_cf->adj_list[BFIv[nbr_vert]][edge_position] = BFIv[vst_vert];
                curr_cf->edge_list[BFIv[nbr_vert]][edge_position] = BFIe[nbr_edge];
            }
        }
        
    }
    
}

/**********************************************************************************/

void flip_graph(struct Graph *graph, struct Graph *flip)
{
    int i;
    int pos1;
    int pos2;
    
    //initiate values of the flip graph
    flip->num_nodes = graph->num_nodes;
    flip->graph_number = graph->graph_number;
    flip->total_edges = graph->total_edges;
    
    for (i = 0; i < graph->num_nodes; i++)
    {
        pos1 = 1;
        pos2 = graph->num_edges[i]-1;
        
        flip->num_edges[i] = graph->num_edges[i];
        
        //keep the first vertex/edge intact
        flip->adj_list[i][0] = graph->adj_list[i][0];
        flip->edge_list[i][0] = graph->edge_list[i][0];
        
        //swap first and last moving up/down the list
        while (pos1 <= pos2)
        {
            flip->adj_list[i][pos1] = graph->adj_list[i][pos2];
            flip->adj_list[i][pos2] = graph->adj_list[i][pos1];
            flip->edge_list[i][pos1] = graph->edge_list[i][pos2];
            flip->edge_list[i][pos2] = graph->edge_list[i][pos1];
            pos1++;
            pos2--;
        }
    }
}

/**********************************************************************************/


int find_pp (int vert, int edge_number, struct Graph *graph)
{
    int i =0;
    while (graph->edge_list[vert][i] != edge_number)
    {
        i++;
    }
    return i;
}

/**********************************************************************************/


void copy_cf(struct Graph *min_cf, struct Graph *curr_cf)
{
    int i;
    int j;
 
    min_cf->graph_number = curr_cf->graph_number;
    min_cf->num_nodes = curr_cf->num_nodes;
    for (i = 0; i < curr_cf->num_nodes; i++)
    {
        min_cf->num_edges[i] = curr_cf->num_edges[i];
        
        for (j = 0; j < curr_cf->num_edges[i]; j++)
        {
            min_cf->adj_list[i][j] = curr_cf->adj_list[i][j];
            min_cf->edge_list[i][j] = curr_cf->edge_list[i][j];
        }
    }
}

/**********************************************************************************/

int cmp_cf (struct Graph *cf1, struct Graph *cf2)
{
    
    //compare each cf by row and determine the lexicographical order
    int i, j, cmp;
    cmp = 0;
    
    if (cf1->num_nodes != cf2->num_nodes)
    {
        fprintf(stderr,"Node values for comparison do not match\n");
        exit(1);
    }
    
    for (i = 0; i < cf1->num_nodes; i++)
    {
        if (cf1->num_edges[i] != cf2->num_edges[i])
        {
            fprintf(stderr,"Row lengths for comparison do not match\n");
            exit(1);
        }
        
        for (j = 0; j < cf1->num_edges[i]; j++)
        {
            if (cf1->adj_list[i][j] < cf2->adj_list[i][j])
            {
                cmp = 1;
                break;
            }
            else if (cf1->adj_list[i][j] > cf2->adj_list[i][j])
            {
                cmp = -1;
                break;
            }
        }
    }
    return cmp;
}

/**********************************************************************************/

//order graph with the lowest number vertex leading each row

void order_graph(struct Graph *graph)
{
    int i, j, pos, min, min_pos;
    
    for (i = 0; i < graph->num_nodes; i++)
    {
        min_pos = 0;
        min = graph->adj_list[i][pos];
        for (j = 1; j < graph->num_edges[i]; j++)
        {
            if (graph->adj_list[i][j] < graph->adj_list[i][min_pos])
            {
                min = graph->adj_list[i][j];
                min_pos = j;
            }
        }
        rotate_row(i,min_pos,graph);
    }
    
}

/**********************************************************************************/

void rotate_row(int vert, int shift, struct Graph *graph)
{
    int i;
    int temp_rowv[NMAX];
    int temp_rowe[NMAX];
    
    int num_edges = graph->num_edges[vert];
    for (i = 0; i < num_edges; i++) {
        temp_rowv[i] = graph->adj_list[vert][(i+shift)%num_edges];
        temp_rowe[i] = graph->edge_list[vert][(i+shift)%num_edges];
        
    }
    for (i = 0; i < num_edges; i++) {
        graph->adj_list[vert][i] = temp_rowv[i];
        graph->edge_list[vert][i] = temp_rowe[i];
        
    }
    
}

/**********************************************************************************/

void print_graph(struct Graph *graph)
{
    //printf("Printing graph # %d\n",graph->graph_number);
    int i;
    int j;
    printf("%d ",graph->num_nodes);
    for (i = 0; i < graph->num_nodes; i++) {
        printf("%d ",graph->num_edges[i]);
        for (j = 0; j < graph->num_edges[i]; j++) {
            printf("%d ", graph->adj_list[i][j]);
        }
    }
    printf("\n");
}









