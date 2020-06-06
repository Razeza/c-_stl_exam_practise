#ifndef UNTITLED1_LIST_H
#define UNTITLED1_LIST_H

#include <iostream>

// Совет один - не пишите на экзамене с аллокаторами, если не уверены на 100%,
// и если не хотите думать, что когда аллоцировать/деаллоцировать...
// Также занимает намного больше место код с аллокаторами, чем код без них,
// поскольку веселье в операторах копирования

// Никогда так не делайте, работает на reinterpret_cast, потому что поздно опомнился
// и не хотелось всё переписывать, написано для ознакомления


template<typename TypeValue>
class list_iterator;

template<typename TypeValue, typename Allocator = std::allocator<TypeValue>>
class list
{
private:
    friend class list_iterator<TypeValue>;

    struct List_header_tail;
    struct List_block;

    std::allocator<List_block>       allocator_block;
    std::allocator<List_header_tail> allocator_head;

    typedef typename std::allocator_traits<std::allocator<List_block>>       alloc_block;
    typedef typename std::allocator_traits<Allocator>                        alloc_val;
    typedef typename std::allocator_traits<std::allocator<List_header_tail>> alloc_head;

    struct List_block
    {
        List_block* prev;
        List_block* next;
        TypeValue*  value;

        
        List_block () = delete;

        List_block (TypeValue* init_val, List_block* init_prev, List_block* init_next = nullptr):
            value (init_val),
            prev  (init_prev),
            next  (init_next)
        { }
    };

    struct List_header_tail
    {
        List_block* prev;
        List_block* next;
        size_t      size;



        List_header_tail ():
            size (0),
            next (nullptr),
            prev (nullptr)
        { }

        List_header_tail (List_block* init):
                size (0),
                next (init),
                prev (init)
        { }

        List_header_tail (List_block* init_val, size_t init_size):
                size (init_size),
                next (init_val),
                prev (init_val)
        { }

        explicit operator List_block ()
        { return List_block (nullptr, prev, next); }
    };

    List_header_tail* head;
    List_header_tail* tail;
    Allocator         allocator;

    void destruct ()
    {
        if (head != nullptr && head->next != nullptr && head->next != reinterpret_cast<List_block*> (tail) )
        {
            auto delete_block = head->next;
            for (size_t i = 0; i < head->size; i++)
            {
                auto tmp_next = delete_block->next;

                alloc_val::destroy      (allocator, delete_block->value);
                alloc_val::deallocate   (allocator, delete_block->value, 1);
                alloc_block::destroy    (allocator_block, delete_block);
                alloc_block::deallocate (allocator_block, delete_block, 1);

                delete_block = tmp_next;
            }



            alloc_head::destroy    (allocator_head, head);
            alloc_head::deallocate (allocator_head, head, 1);
            alloc_head::destroy    (allocator_head, tail);
            alloc_head::deallocate (allocator_head, tail, 1);
        }
    }

public:

    list ():
        head    (alloc_head::allocate (allocator_head, 1)),
        tail    (alloc_head::allocate (allocator_head, 1))
    { alloc_head::construct (allocator_head, head, reinterpret_cast<List_block*> (tail) ), alloc_head::construct (allocator_head, tail,
                                                                                                                  reinterpret_cast<List_block*> (head) );}

    list (const list& init):
        head            (alloc_head::allocate  (allocator_head, 1) ),
        tail            (alloc_head::allocate  (allocator_head, 1) ),
        allocator       (std::allocator_traits<Allocator>::select_on_container_copy_construction (init.allocator))
    {
        auto allocated_memory = alloc_val::allocate (allocator, init.head->size);
        alloc_head::construct (allocator_head, head, alloc_block::allocate (allocator_block, init.head->size), init.head->size);
        tail->prev = head->next + init.head->size - 1;


        for (size_t i = 0; i < init.head->size; i++)
        {
            alloc_block::construct  (allocator_block, head->next + i, allocated_memory + i,
                     (i == 0               ? reinterpret_cast<List_block*> (head) : head->next + i - 1),
                     (i == init.head->size ? reinterpret_cast<List_block*> (tail) : head->next + i + 1) );

            alloc_val::construct (allocator, allocated_memory + i, *(head->next + i)->value);
        }
    }

    list (list&& init):
        head            (init.head),
        tail            (init.tail),
        allocator       (std::move (init.allocator) ),
        allocator_head  (std::move (init.allocator_head) ),
        allocator_block (std::move (init.allocator_block) )
    {
        init.head = nullptr;
        init.tail = nullptr;
    }

    list& operator= (const list& init)
    {
        if (this == &init)
        {
            return *this;
        }

        bool copy_allocator_required = alloc_val::propagate_on_container_copy_assignment::value;
        bool realloc_required        = (copy_allocator_required && init.allocator != allocator) || head->size <= init.head->size;

        auto destroy = head->next;
        for (size_t i = 0; i < head->size; i++)
        {
            auto tmp = destroy->next;

            alloc_val::destroy   (allocator, (destroy)->value);
            alloc_block::destroy (allocator_block, destroy);

            if (realloc_required)
            {
                alloc_val::deallocate   (allocator, destroy->value, 1);
                alloc_block::deallocate (allocator_block, destroy, 1);
            }

            destroy = destroy->tmp;
        }


        if (copy_allocator_required)
        {
            allocator       = init.allocator;
            allocator_head  = init.allocator_head;
            allocator_block = init.allocator_block;
        }

        if (realloc_required)
        {
            head->first_block        = alloc_block::allocate (allocator_block, init.head->size);
            head->first_block->value = alloc_val::allocate   (allocator, init.head->size);
        }

        head->size = init.head->size;


        auto construct = head->next;
        for (size_t i = 0; i < init.head->size; i++)
        {
            alloc_val::construct (allocator, construct->value, *(construct)->value);

            alloc_block::construct  (allocator_block, construct, construct->value,
                                     (i == 0               ? nullptr : construct - 1),
                                     (i == init.head->size ? nullptr : construct + 1) );

            if (i == init.head->size - 1)
            {
                tail->prev = construct;
            }

            construct = construct->next;
        }

        return *this;
    }

    list& operator= (list&& init)
    {
        if (this == &init)
        {
            return *this;
        }

        bool move_allocator_required = alloc_val::propagate_on_container_move_assignment::value;

        if (init.allocator != allocator && move_allocator_required)
        {
            auto destroy = head->next;
            for (size_t i = 0; i < head->size; i++)
            {
                auto tmp = destroy->next;

                alloc_val::destroy   (allocator, (destroy)->value);
                alloc_block::destroy (allocator_block, destroy);

                if (head->size <= init.head->size)
                {
                    alloc_val::deallocate   (allocator, destroy->value, head->size);
                    alloc_block::deallocate (allocator_block, destroy, head->size);
                }

                destroy = tmp;
            }

            if (head->size <= init.head->size)
            {
                head->next        = alloc_block::allocate (allocator_block, init.head->size);
                head->next->value = alloc_val::allocate   (allocator, init.head->size);
            }

            head->size = init.head->size;

            auto construct = head->next;
            for (size_t i = 0; i < init.head->size; i++)
            {
                alloc_val::construct (allocator, construct->value, *(construct)->value);

                alloc_block::construct  (allocator_block, construct, construct->value,
                                         (i == 0               ? nullptr : construct - 1),
                                         (i == init.head->size ? nullptr : construct + 1) );

                if (i == init.head->size - 1)
                {
                    tail->prev = construct;
                }

                construct = construct->next;
            }

        }
        else
        {
            destruct ();

            head = init.head;
            tail = init.tail;

            if (move_allocator_required)
            {
                allocator = std::move (init.allocator);
                allocator_head = std::move (init.allocator_head);
                allocator_block = std::move (init.allocator_block);
            }

            init.head = nullptr;
            init.tail = nullptr;
        }

        return *this;
    }

    ~list ()
    { destruct (); }


    list_iterator<TypeValue> begin ()
    { return list_iterator<TypeValue> (head->next); }

    const list_iterator<TypeValue> begin () const
    { return list_iterator<TypeValue> (head->next); }

    list_iterator<TypeValue> end ()
    { return list_iterator<TypeValue> (reinterpret_cast<List_block*> (tail) ); }

    const list_iterator<TypeValue> end () const
    { return list_iterator<TypeValue> (reinterpret_cast<List_block*> (tail) ); }

private:

    template<typename... Arguments>
    List_block* create_node (Arguments... arguments)
    {
        List_block* alloc_place = alloc_block::allocate (allocator_block, 1);

        alloc_block::construct (allocator_block, alloc_place, alloc_val::allocate (allocator, 1), nullptr );
        alloc_val::construct (allocator, alloc_place->value, std::forward<Arguments> (arguments)... );
        return alloc_place;
    }

    template<typename... Arguments>
    void insert_iter (const list_iterator<TypeValue>& iterator, Arguments... arguments)
    {
        auto new_node = create_node (std::forward<Arguments> (arguments)... );

        new_node->prev          = iterator.it->prev;
        new_node->next          = iterator.it;
        iterator.it->prev->next = new_node;
        iterator.it->prev       = new_node;


        head->size++;
    }

public:
    void push_back (const TypeValue& init)
    {
        insert_iter (end (), init);
    }

    void push_back (const TypeValue&& init)
    {
        insert_iter (end (), std::move (init) );
    }

    void push_front (const TypeValue& init)
    {
        insert_iter (begin (), init);
    }

    void push_front (const TypeValue&& init)
    {
        insert_iter (begin (), std::move (init) );
    }
};

template<typename TypeValue>
class list_iterator
{
private:
    friend class list<TypeValue>;

    typename list<TypeValue>::List_block* it;

    using it_traits = std::iterator_traits<typename list<TypeValue>::List_block*>;
public:

    typedef typename std::bidirectional_iterator_tag iterator_category;
    typedef typename it_traits::value_type  	     value_type;
    typedef typename it_traits::difference_type      difference_type;
    typedef typename it_traits::reference 	         reference;
    typedef typename it_traits::pointer   	         pointer;

    list_iterator (typename list<TypeValue>::List_block* init):
        it (init)
    { }

    TypeValue& operator* () const
    { return *it->value; }

    TypeValue* operator->() const
    { return it->value; }

    list_iterator& operator++ ()
    {
        it = it->next;
        return *this;
    }

    list_iterator& operator-- ()
    {
        it = it->prev;
        return *this;
    }

    list_iterator operator++(int)
    {
        auto it_tmp = it;
        it = it->next;
        return list_iterator(it_tmp);
    }

    list_iterator operator--(int)
    {
        auto it_tmp = it;
        it = it->prev;
        return list_iterator(it_tmp);
    }

    bool operator== (const list_iterator<TypeValue>& other)
    { return it == other.it; }

    bool operator!= (const list_iterator<TypeValue>& other)
    { return it != other.it; }
};







#endif //UNTITLED1_LIST_H
